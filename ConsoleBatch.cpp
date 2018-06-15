/*
    Scan Tailor - Interactive post-processing tool for scanned pages.

    ConsoleBatch - Batch processing scanned pages from command line.
    Copyright (C) 2011 Petr Kovar <pejuko@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <cassert>
#include <iostream>
#include <vector>

#include "FileNameDisambiguator.h"
#include "LoadFileTask.h"
#include "OutputFileNameGenerator.h"
#include "PageSelectionAccessor.h"
#include "PageSequence.h"
#include "ProcessingTaskQueue.h"
#include "ProjectPages.h"
#include "ProjectReader.h"
#include "ProjectWriter.h"
#include "StageSequence.h"
#include "Utils.h"

#include "filters/deskew/CacheDrivenTask.h"
#include "filters/deskew/Task.h"
#include "filters/fix_orientation/CacheDrivenTask.h"
#include "filters/fix_orientation/Settings.h"
#include "filters/fix_orientation/Task.h"
#include "filters/output/CacheDrivenTask.h"
#include "filters/output/Settings.h"
#include "filters/output/Task.h"
#include "filters/page_layout/CacheDrivenTask.h"
#include "filters/page_layout/Task.h"
#include "filters/page_split/CacheDrivenTask.h"
#include "filters/page_split/Settings.h"
#include "filters/page_split/Task.h"
#include "filters/select_content/CacheDrivenTask.h"
#include "filters/select_content/Task.h"

#include "ConsoleBatch.h"

ConsoleBatch::ConsoleBatch(const std::vector<ImageFileInfo>& images,
                           const QString& output_directory,
                           const Qt::LayoutDirection layout)
    : batch(true),
      debug(true),
      m_ptrDisambiguator(new FileNameDisambiguator),
      m_ptrPages(new ProjectPages(images, ProjectPages::AUTO_PAGES, layout)) {
  const PageSelectionAccessor accessor(nullptr);  // Won't really be used anyway.
  m_ptrStages = make_intrusive<StageSequence>(m_ptrPages, accessor);
  // m_ptrThumbnailCache = make_intrusive<ThumbnailPixmapCache>(output_dir+"/cache/thumbs",
  // QSize(200,200), 40, 5);
  m_ptrThumbnailCache = Utils::createThumbnailCache(output_directory);
  m_outFileNameGen = OutputFileNameGenerator(m_ptrDisambiguator, output_directory, m_ptrPages->layoutDirection());
}

ConsoleBatch::ConsoleBatch(const QString project_file) : batch(true), debug(true) {
  QFile file(project_file);
  if (!file.open(QIODevice::ReadOnly)) {
    throw std::runtime_error("ConsoleBatch: Unable to open the project file.");
  }

  QDomDocument doc;
  if (!doc.setContent(&file)) {
    throw std::runtime_error("ConsoleBatch: The project file is broken.");
  }

  file.close();

  m_ptrReader = std::make_unique<ProjectReader>(doc);
  m_ptrPages = m_ptrReader->pages();

  const PageSelectionAccessor accessor(nullptr);  // Won't be used anyway.
  m_ptrDisambiguator = m_ptrReader->namingDisambiguator();

  m_ptrStages = make_intrusive<StageSequence>(m_ptrPages, accessor);
  m_ptrReader->readFilterSettings(m_ptrStages->filters());

  const CommandLine& cli = CommandLine::get();
  QString output_directory = m_ptrReader->outputDirectory();
  if (!cli.outputDirectory().isEmpty()) {
    output_directory = cli.outputDirectory();
  }
  // m_ptrThumbnailCache = make_intrusive<    // ThumbnailPixmapCache>(output_directory+"/cache/thumbs",
  // QSize(200,200), 40, 5);
  m_ptrThumbnailCache = Utils::createThumbnailCache(output_directory);
  m_outFileNameGen = OutputFileNameGenerator(m_ptrDisambiguator, output_directory, m_ptrPages->layoutDirection());
}

BackgroundTaskPtr ConsoleBatch::createCompositeTask(const PageInfo& page, const int last_filter_idx) {
  intrusive_ptr<fix_orientation::Task> fix_orientation_task;
  intrusive_ptr<page_split::Task> page_split_task;
  intrusive_ptr<deskew::Task> deskew_task;
  intrusive_ptr<select_content::Task> select_content_task;
  intrusive_ptr<page_layout::Task> page_layout_task;
  intrusive_ptr<output::Task> output_task;

  if (batch) {
    debug = false;
  }

  if (last_filter_idx >= m_ptrStages->outputFilterIdx()) {
    output_task
        = m_ptrStages->outputFilter()->createTask(page.id(), m_ptrThumbnailCache, m_outFileNameGen, batch, debug);
    debug = false;
  }
  if (last_filter_idx >= m_ptrStages->pageLayoutFilterIdx()) {
    page_layout_task = m_ptrStages->pageLayoutFilter()->createTask(page.id(), output_task, batch, debug);
    debug = false;
  }
  if (last_filter_idx >= m_ptrStages->selectContentFilterIdx()) {
    select_content_task = m_ptrStages->selectContentFilter()->createTask(page.id(), page_layout_task, batch, debug);
    debug = false;
  }
  if (last_filter_idx >= m_ptrStages->deskewFilterIdx()) {
    deskew_task = m_ptrStages->deskewFilter()->createTask(page.id(), select_content_task, batch, debug);
    debug = false;
  }
  if (last_filter_idx >= m_ptrStages->pageSplitFilterIdx()) {
    page_split_task = m_ptrStages->pageSplitFilter()->createTask(page, deskew_task, batch, debug);
    debug = false;
  }
  if (last_filter_idx >= m_ptrStages->fixOrientationFilterIdx()) {
    fix_orientation_task = m_ptrStages->fixOrientationFilter()->createTask(page.id(), page_split_task, batch);
    debug = false;
  }
  assert(fix_orientation_task);

  return make_intrusive<LoadFileTask>(BackgroundTask::BATCH, page, m_ptrThumbnailCache, m_ptrPages,
                                      fix_orientation_task);
}  // ConsoleBatch::createCompositeTask

// process the image vector **images** and save output to **output_dir**
void ConsoleBatch::process() {
  const CommandLine& cli = CommandLine::get();

  int startFilterIdx = m_ptrStages->fixOrientationFilterIdx();
  if (cli.hasStartFilterIdx()) {
    unsigned int sf = cli.getStartFilterIdx();
    if ((sf < 0) || (sf >= m_ptrStages->filters().size())) {
      throw std::runtime_error("ConsoleBatch: Start filter out of range");
    }
    startFilterIdx = sf;
  }

  int endFilterIdx = m_ptrStages->outputFilterIdx();
  if (cli.hasEndFilterIdx()) {
    unsigned int ef = cli.getEndFilterIdx();
    if ((ef < 0) || (ef >= m_ptrStages->filters().size())) {
      throw std::runtime_error("ConsoleBatch: End filter out of range");
    }
    endFilterIdx = ef;
  }

  for (int j = startFilterIdx; j <= endFilterIdx; j++) {
    if (cli.isVerbose()) {
      std::cout << "Filter: " << (j + 1) << "\n";
    }

    PageSequence page_sequence = m_ptrPages->toPageSequence(PAGE_VIEW);
    setupFilter(j, page_sequence.selectAll());
    for (unsigned i = 0; i < page_sequence.numPages(); i++) {
      PageInfo page = page_sequence.pageAt(i);
      if (cli.isVerbose()) {
        std::cout << "\tProcessing: " << page.imageId().filePath().toLatin1().constData() << "\n";
      }
      BackgroundTaskPtr bgTask = createCompositeTask(page, j);
      (*bgTask)();
    }
  }

  for (int j = endFilterIdx + 1; j <= m_ptrStages->count(); j++) {
    PageSequence page_sequence = m_ptrPages->toPageSequence(PAGE_VIEW);
    setupFilter(j, page_sequence.selectAll());
  }

  for (int j = 0; j <= endFilterIdx; j++) {
    m_ptrStages->filterAt(j)->updateStatistics();
  }
}  // ConsoleBatch::process

void ConsoleBatch::saveProject(const QString project_file) {
  PageInfo fpage = m_ptrPages->toPageSequence(PAGE_VIEW).pageAt(0);
  SelectedPage sPage(fpage.id(), IMAGE_VIEW);
  ProjectWriter writer(m_ptrPages, sPage, m_outFileNameGen);
  writer.write(project_file, m_ptrStages->filters());
}

void ConsoleBatch::setupFilter(int idx, std::set<PageId> allPages) {
  if (idx == m_ptrStages->fixOrientationFilterIdx()) {
    setupFixOrientation(allPages);
  } else if (idx == m_ptrStages->pageSplitFilterIdx()) {
    setupPageSplit(allPages);
  } else if (idx == m_ptrStages->deskewFilterIdx()) {
    setupDeskew(allPages);
  } else if (idx == m_ptrStages->selectContentFilterIdx()) {
    setupSelectContent(allPages);
  } else if (idx == m_ptrStages->pageLayoutFilterIdx()) {
    setupPageLayout(allPages);
  } else if (idx == m_ptrStages->outputFilterIdx()) {
    setupOutput(allPages);
  }
}

void ConsoleBatch::setupFixOrientation(std::set<PageId> allPages) {
  intrusive_ptr<fix_orientation::Filter> fix_orientation = m_ptrStages->fixOrientationFilter();
  const CommandLine& cli = CommandLine::get();

  for (std::set<PageId>::iterator i = allPages.begin(); i != allPages.end(); i++) {
    PageId page = *i;

    OrthogonalRotation rotation;
    // FIX ORIENTATION FILTER
    if (cli.hasOrientation()) {
      switch (cli.getOrientation()) {
        case CommandLine::LEFT:
          rotation.prevClockwiseDirection();
          break;
        case CommandLine::RIGHT:
          rotation.nextClockwiseDirection();
          break;
        case CommandLine::UPSIDEDOWN:
          rotation.nextClockwiseDirection();
          rotation.nextClockwiseDirection();
          break;
        default:
          break;
      }
      fix_orientation->getSettings()->applyRotation(page.imageId(), rotation);
    }
  }
}

void ConsoleBatch::setupPageSplit(std::set<PageId> allPages) {
  intrusive_ptr<page_split::Filter> page_split = m_ptrStages->pageSplitFilter();
  const CommandLine& cli = CommandLine::get();

  // PAGE SPLIT
  if (cli.hasLayout()) {
    page_split->getSettings()->setLayoutTypeForAllPages(cli.getLayout());
  }
}

void ConsoleBatch::setupDeskew(std::set<PageId> allPages) {
  intrusive_ptr<deskew::Filter> deskew = m_ptrStages->deskewFilter();
  const CommandLine& cli = CommandLine::get();

  for (std::set<PageId>::iterator i = allPages.begin(); i != allPages.end(); i++) {
    PageId page = *i;
    // DESKEW FILTER
    OrthogonalRotation rotation;
    if (cli.hasDeskewAngle() || cli.hasDeskew()) {
      double angle = 0.0;
      if (cli.hasDeskewAngle()) {
        angle = cli.getDeskewAngle();
      }
      deskew::Dependencies deps(QPolygonF(), rotation);
      deskew::Params params(angle, deps, MODE_MANUAL);
      deskew->getSettings()->setPageParams(page, params);
    }
  }

  if (cli.hasSkewDeviation()) {
    deskew->getSettings()->setMaxDeviation(cli.getSkewDeviation());
  }
}

void ConsoleBatch::setupSelectContent(std::set<PageId> allPages) {
  intrusive_ptr<select_content::Filter> select_content = m_ptrStages->selectContentFilter();
  const CommandLine& cli = CommandLine::get();

  for (std::set<PageId>::iterator i = allPages.begin(); i != allPages.end(); i++) {
    PageId page = *i;
    select_content::Dependencies deps;

    select_content::Params params(deps);
    std::unique_ptr<select_content::Params> old_params = select_content->getSettings()->getPageParams(page);

    if (old_params) {
      params = *old_params;
    }
    // SELECT CONTENT FILTER
    if (cli.hasContentRect()) {
      params.setContentRect(cli.getContentRect());
    }

    params.setContentDetect(cli.isContentDetectionEnabled());
    params.setPageDetect(cli.isPageDetectionEnabled());
    params.setFineTuneCorners(cli.isFineTuningEnabled());
    if (cli.hasPageBorders()) {
      params.setPageBorders(cli.getPageBorders());
    }

    select_content->getSettings()->setPageParams(page, params);
  }

  if (cli.hasContentDeviation()) {
    select_content->getSettings()->setMaxDeviation(cli.getContentDeviation());
  }

  if (cli.hasPageDetectionBox()) {
    select_content->getSettings()->setPageDetectionBox(cli.getPageDetectionBox());
  }

  if (cli.hasPageDetectionTolerance()) {
    select_content->getSettings()->setPageDetectionTolerance(cli.getPageDetectionTolerance());
  }
}  // ConsoleBatch::setupSelectContent

void ConsoleBatch::setupPageLayout(std::set<PageId> allPages) {
  intrusive_ptr<page_layout::Filter> page_layout = m_ptrStages->pageLayoutFilter();
  const CommandLine& cli = CommandLine::get();
  QMap<QString, float> img_cache;

  for (std::set<PageId>::iterator i = allPages.begin(); i != allPages.end(); i++) {
    PageId page = *i;

    // PAGE LAYOUT FILTER
    page_layout::Alignment alignment = cli.getAlignment();
    if (cli.hasMatchLayoutTolerance()) {
      const QString path = page.imageId().filePath();
      if (!img_cache.contains(path)) {
        QImage img(path);
        img_cache[path] = float(img.width()) / float(img.height());
      }
      float imgAspectRatio = img_cache[path];
      float tolerance = cli.getMatchLayoutTolerance();
      std::vector<float> diffs;
      for (std::set<PageId>::iterator pi = allPages.begin(); pi != allPages.end(); pi++) {
        ImageId pimageId = pi->imageId();
        QString ppath = pimageId.filePath();
        if (!img_cache.contains(ppath)) {
          QImage img(ppath);
          img_cache[ppath] = float(img.width()) / float(img.height());
        }
        float pimgAspectRatio = img_cache[ppath];
        float diff = imgAspectRatio - pimgAspectRatio;
        if (diff < 0.0) {
          diff *= -1;
        }
        diffs.push_back(diff);
      }
      unsigned bad_diffs = 0;
      for (unsigned j = 0; j < diffs.size(); j++) {
        if (diffs[j] > tolerance) {
          bad_diffs += 1;
        }
      }
      if (bad_diffs > (diffs.size() / 2)) {
        alignment.setNull(true);
      }
    }
    if (cli.hasMargins()) {
      page_layout->getSettings()->setHardMarginsMM(page, cli.getMargins());
    }
    if (cli.hasAlignment()) {
      page_layout->getSettings()->setPageAlignment(page, alignment);
    }
  }
}  // ConsoleBatch::setupPageLayout

void ConsoleBatch::setupOutput(std::set<PageId> allPages) {
  intrusive_ptr<output::Filter> output = m_ptrStages->outputFilter();
  const CommandLine& cli = CommandLine::get();

  for (std::set<PageId>::iterator i = allPages.begin(); i != allPages.end(); i++) {
    PageId page = *i;

    // OUTPUT FILTER
    output::Params params(output->getSettings()->getParams(page));
    if (cli.hasOutputDpi()) {
      Dpi outputDpi = cli.getOutputDpi();
      params.setOutputDpi(outputDpi);
    }

    if (cli.hasPictureShape()) {
      params.setPictureShape(cli.getPictureShape());
    }

    output::ColorParams colorParams = params.colorParams();
    if (cli.hasColorMode()) {
      colorParams.setColorMode(cli.getColorMode());
    }

    if (cli.hasFillMargins() || cli.hasNormalizeIllumination()) {
      output::ColorCommonOptions cgo;
      if (cli.hasFillMargins()) {
        cgo.setFillMargins(true);
      }
      if (cli.hasNormalizeIllumination()) {
        cgo.setNormalizeIllumination(true);
      }
      colorParams.setColorCommonOptions(cgo);
    }

    if (cli.hasThreshold()) {
      output::BlackWhiteOptions bwo;
      bwo.setThresholdAdjustment(cli.getThreshold());
      colorParams.setBlackWhiteOptions(bwo);
    }

    params.setColorParams(colorParams);

    if (cli.hasDespeckle()) {
      params.setDespeckleLevel(cli.getDespeckleLevel());
    }

    if (cli.hasDewarping()) {
      params.setDewarpingMode(cli.getDewarpingMode());
    }
    if (cli.hasDepthPerception()) {
      params.setDepthPerception(cli.getDepthPerception());
    }

    output->getSettings()->setParams(page, params);
  }
}  // ConsoleBatch::setupOutput
