/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C)  Joseph Artsimovich <joseph.artsimovich@gmail.com>

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

#include "Task.h"
#include <UnitsProvider.h>
#include <QDir>
#include <boost/bind.hpp>
#include <utility>
#include "CommandLine.h"
#include "DebugImages.h"
#include "DespeckleState.h"
#include "DespeckleView.h"
#include "DespeckleVisualization.h"
#include "DewarpingView.h"
#include "Dpm.h"
#include "ErrorWidget.h"
#include "FillZoneComparator.h"
#include "FillZoneEditor.h"
#include "Filter.h"
#include "FilterData.h"
#include "FilterUiInterface.h"
#include "ImageLoader.h"
#include "ImageView.h"
#include "OptionsWidget.h"
#include "OutputGenerator.h"
#include "PictureZoneComparator.h"
#include "PictureZoneEditor.h"
#include "RenderParams.h"
#include "Settings.h"
#include "TabbedImageView.h"
#include "TaskStatus.h"
#include "ThumbnailPixmapCache.h"
#include "Utils.h"
#include "dewarping/DewarpingPointMapper.h"
#include "imageproc/PolygonUtils.h"

using namespace imageproc;
using namespace dewarping;

namespace output {
class Task::UiUpdater : public FilterResult {
  Q_DECLARE_TR_FUNCTIONS(output::Task::UiUpdater)
 public:
  UiUpdater(intrusive_ptr<Filter> filter,
            intrusive_ptr<Settings> settings,
            std::unique_ptr<DebugImages> dbg_img,
            const Params& params,
            const ImageTransformation& xform,
            const QTransform& postTransform,
            const QRect& virt_content_rect,
            const PageId& page_id,
            const QImage& orig_image,
            const QImage& output_image,
            const BinaryImage& picture_mask,
            const DespeckleState& despeckle_state,
            const DespeckleVisualization& despeckle_visualization,
            bool batch,
            bool debug);

  void updateUI(FilterUiInterface* ui) override;

  intrusive_ptr<AbstractFilter> filter() override { return m_ptrFilter; }

 private:
  intrusive_ptr<Filter> m_ptrFilter;
  intrusive_ptr<Settings> m_ptrSettings;
  std::unique_ptr<DebugImages> m_ptrDbg;
  Params m_params;
  ImageTransformation m_xform;
  QTransform m_outputPostTransform;
  QRect m_virtContentRect;
  PageId m_pageId;
  QImage m_origImage;
  QImage m_downscaledOrigImage;
  QImage m_outputImage;
  QImage m_downscaledOutputImage;
  BinaryImage m_pictureMask;
  DespeckleState m_despeckleState;
  DespeckleVisualization m_despeckleVisualization;
  bool m_batchProcessing;
  bool m_debug;
};


Task::Task(intrusive_ptr<Filter> filter,
           intrusive_ptr<Settings> settings,
           intrusive_ptr<ThumbnailPixmapCache> thumbnail_cache,
           const PageId& page_id,
           const OutputFileNameGenerator& out_file_name_gen,
           const ImageViewTab last_tab,
           const bool batch,
           const bool debug)
    : m_ptrFilter(std::move(filter)),
      m_ptrSettings(std::move(settings)),
      m_ptrThumbnailCache(std::move(thumbnail_cache)),
      m_pageId(page_id),
      m_outFileNameGen(out_file_name_gen),
      m_lastTab(last_tab),
      m_batchProcessing(batch),
      m_debug(debug) {
  if (debug) {
    m_ptrDbg = std::make_unique<DebugImages>();
  }
}

Task::~Task() = default;

FilterResultPtr Task::process(const TaskStatus& status, const FilterData& data, const QPolygonF& content_rect_phys) {
  status.throwIfCancelled();

  Params params(m_ptrSettings->getParams(m_pageId));

  RenderParams render_params(params.colorParams(), params.splittingOptions());
  const QString out_file_path(m_outFileNameGen.filePathFor(m_pageId));
  const QFileInfo out_file_info(out_file_path);

  ImageTransformation new_xform(data.xform());
  new_xform.postScaleToDpi(params.outputDpi());

  const QString foreground_dir(Utils::foregroundDir(m_outFileNameGen.outDir()));
  const QString background_dir(Utils::backgroundDir(m_outFileNameGen.outDir()));
  const QString original_background_dir(Utils::originalBackgroundDir(m_outFileNameGen.outDir()));
  const QString foreground_file_path(QDir(foreground_dir).absoluteFilePath(out_file_info.fileName()));
  const QString background_file_path(QDir(background_dir).absoluteFilePath(out_file_info.fileName()));
  const QString original_background_file_path(QDir(original_background_dir).absoluteFilePath(out_file_info.fileName()));
  const QFileInfo foreground_file_info(foreground_file_path);
  const QFileInfo background_file_info(background_file_path);
  const QFileInfo original_background_file_info(original_background_file_path);

  const QString automask_dir(Utils::automaskDir(m_outFileNameGen.outDir()));
  const QString automask_file_path(QDir(automask_dir).absoluteFilePath(out_file_info.fileName()));
  QFileInfo automask_file_info(automask_file_path);

  const QString speckles_dir(Utils::specklesDir(m_outFileNameGen.outDir()));
  const QString speckles_file_path(QDir(speckles_dir).absoluteFilePath(out_file_info.fileName()));
  QFileInfo speckles_file_info(speckles_file_path);

  const bool need_picture_editor = render_params.mixedOutput() && !m_batchProcessing;
  const bool need_speckles_image
      = params.despeckleLevel() != DESPECKLE_OFF && render_params.needBinarization() && !m_batchProcessing;

  {
    std::unique_ptr<OutputParams> stored_output_params(m_ptrSettings->getOutputParams(m_pageId));
    if (stored_output_params != nullptr) {
      if (stored_output_params->outputImageParams().getPictureShapeOptions() != params.pictureShapeOptions()) {
        // if picture shape options changed, reset auto picture zones
        OutputProcessingParams outputProcessingParams = m_ptrSettings->getOutputProcessingParams(m_pageId);
        outputProcessingParams.setAutoZonesFound(false);
        m_ptrSettings->setOutputProcessingParams(m_pageId, outputProcessingParams);
      }
    }
  }

  OutputGenerator generator(params.outputDpi(), params.colorParams(), params.splittingOptions(),
                            params.pictureShapeOptions(), params.dewarpingOptions(),
                            m_ptrSettings->getOutputProcessingParams(m_pageId), params.despeckleLevel(), new_xform,
                            content_rect_phys);

  OutputImageParams new_output_image_params(
      generator.outputImageSize(), generator.outputContentRect(), new_xform, params.outputDpi(), params.colorParams(),
      params.splittingOptions(), params.dewarpingOptions(), params.distortionModel(), params.depthPerception(),
      params.despeckleLevel(), params.pictureShapeOptions(), m_ptrSettings->getOutputProcessingParams(m_pageId),
      params.isBlackOnWhite());

  ZoneSet new_picture_zones(m_ptrSettings->pictureZonesForPage(m_pageId));
  const ZoneSet new_fill_zones(m_ptrSettings->fillZonesForPage(m_pageId));

  bool need_reprocess = false;
  do {  // Just to be able to break from it.
    std::unique_ptr<OutputParams> stored_output_params(m_ptrSettings->getOutputParams(m_pageId));

    if (!stored_output_params) {
      need_reprocess = true;
      break;
    }

    if (!stored_output_params->outputImageParams().matches(new_output_image_params)) {
      need_reprocess = true;
      break;
    }

    if (!PictureZoneComparator::equal(stored_output_params->pictureZones(), new_picture_zones)) {
      need_reprocess = true;
      break;
    }

    if (!FillZoneComparator::equal(stored_output_params->fillZones(), new_fill_zones)) {
      need_reprocess = true;
      break;
    }

    if (!render_params.splitOutput()) {
      if (!out_file_info.exists()) {
        need_reprocess = true;
        break;
      }

      if (!stored_output_params->outputFileParams().matches(OutputFileParams(out_file_info))) {
        need_reprocess = true;
        break;
      }
    } else {
      if (!foreground_file_info.exists() || !background_file_info.exists()) {
        need_reprocess = true;
        break;
      }
      if (!(stored_output_params->foregroundFileParams().matches(OutputFileParams(foreground_file_info)))
          || !(stored_output_params->backgroundFileParams().matches(OutputFileParams(background_file_info)))) {
        need_reprocess = true;
        break;
      }

      if (render_params.originalBackground()) {
        if (!original_background_file_info.exists()) {
          need_reprocess = true;
          break;
        }
        if (!(stored_output_params->originalBackgroundFileParams().matches(
                OutputFileParams(original_background_file_info)))) {
          need_reprocess = true;
          break;
        }
      }
    }

    if (need_picture_editor) {
      if (!automask_file_info.exists()) {
        need_reprocess = true;
        break;
      }

      if (!stored_output_params->automaskFileParams().matches(OutputFileParams(automask_file_info))) {
        need_reprocess = true;
        break;
      }
    }

    if (need_speckles_image) {
      if (!speckles_file_info.exists()) {
        need_reprocess = true;
        break;
      }
      if (!stored_output_params->specklesFileParams().matches(OutputFileParams(speckles_file_info))) {
        need_reprocess = true;
        break;
      }
    }
  } while (false);

  QImage out_img;
  BinaryImage automask_img;
  BinaryImage speckles_img;

  if (!need_reprocess) {
    QFile out_file(out_file_path);
    if (out_file.open(QIODevice::ReadOnly)) {
      out_img = ImageLoader::load(out_file, 0);
    }
    if (out_img.isNull() && render_params.splitOutput()) {
      QImage foreground_image;
      QImage background_image;
      QFile foreground_file(foreground_file_path);
      QFile background_file(background_file_path);
      if (foreground_file.open(QIODevice::ReadOnly)) {
        foreground_image = ImageLoader::load(foreground_file, 0);
      }
      if (background_file.open(QIODevice::ReadOnly)) {
        background_image = ImageLoader::load(background_file, 0);
      }

      SplitImage tmpSplitImage;
      if (!render_params.originalBackground()) {
        tmpSplitImage = SplitImage(foreground_image, background_image);
      } else {
        QImage original_background_image;
        QFile original_background_file(original_background_file_path);
        if (original_background_file.open(QIODevice::ReadOnly)) {
          original_background_image = ImageLoader::load(original_background_file, 0);
        }
        tmpSplitImage = SplitImage(foreground_image, background_image, original_background_image);
      }
      if (!tmpSplitImage.isNull()) {
        out_img = tmpSplitImage.toImage();
      }
    }
    need_reprocess = out_img.isNull();

    if (need_picture_editor && !need_reprocess) {
      QFile automask_file(automask_file_path);
      if (automask_file.open(QIODevice::ReadOnly)) {
        automask_img = BinaryImage(ImageLoader::load(automask_file, 0));
      }
      need_reprocess = automask_img.isNull() || automask_img.size() != out_img.size();
    }

    if (need_speckles_image && !need_reprocess) {
      QFile speckles_file(speckles_file_path);
      if (speckles_file.open(QIODevice::ReadOnly)) {
        speckles_img = BinaryImage(ImageLoader::load(speckles_file, 0));
      }
      need_reprocess = speckles_img.isNull();
    }
  }

  if (need_reprocess) {
    // Even in batch processing mode we should still write automask, because it
    // will be needed when we view the results back in interactive mode.
    // The same applies even more to speckles file, as we need it not only
    // for visualization purposes, but also for re-doing despeckling at
    // different levels without going through the whole output generation process.
    const bool write_automask = render_params.mixedOutput();
    const bool write_speckles_file = params.despeckleLevel() != DESPECKLE_OFF && render_params.needBinarization();

    automask_img = BinaryImage();
    speckles_img = BinaryImage();

    // OutputGenerator will write a new distortion model
    // there, if dewarping mode is AUTO.
    DistortionModel distortion_model;
    if (params.dewarpingOptions().dewarpingMode() == MANUAL) {
      distortion_model = params.distortionModel();
    }

    SplitImage splitImage;

    out_img
        = generator.process(status, data, new_picture_zones, new_fill_zones, distortion_model, params.depthPerception(),
                            write_automask ? &automask_img : nullptr, write_speckles_file ? &speckles_img : nullptr,
                            m_ptrDbg.get(), m_pageId, m_ptrSettings, &splitImage);

    if (((params.dewarpingOptions().dewarpingMode() == AUTO) || (params.dewarpingOptions().dewarpingMode() == MARGINAL))
        && distortion_model.isValid()) {
      // A new distortion model was generated.
      // We need to save it to be able to modify it manually.
      params.setDistortionModel(distortion_model);
      m_ptrSettings->setParams(m_pageId, params);
      new_output_image_params.setDistortionModel(distortion_model);
    }

    // Saving refreshed params and output processing params.
    new_output_image_params.setBlackOnWhite(m_ptrSettings->getParams(m_pageId).isBlackOnWhite());
    new_output_image_params.setOutputProcessingParams(m_ptrSettings->getOutputProcessingParams(m_pageId));

    bool invalidate_params = false;

    if (render_params.splitOutput()) {
      QDir().mkdir(foreground_dir);
      QDir().mkdir(background_dir);

      if (!TiffWriter::writeImage(foreground_file_path, splitImage.getForegroundImage())
          || !TiffWriter::writeImage(background_file_path, splitImage.getBackgroundImage())) {
        invalidate_params = true;
      }

      if (render_params.originalBackground()) {
        QDir().mkdir(original_background_dir);

        if (!TiffWriter::writeImage(original_background_file_path, splitImage.getOriginalBackgroundImage())) {
          invalidate_params = true;
        }
      }

      out_img = splitImage.toImage();
      splitImage = SplitImage();
    } else {
      // Remove layers if the mode was changed.
      QFile(foreground_file_path).remove();
      QFile(background_file_path).remove();
      QFile(original_background_file_path).remove();
    }
    if (!TiffWriter::writeImage(out_file_path, out_img)) {
      invalidate_params = true;
    } else {
      deleteMutuallyExclusiveOutputFiles();
    }

    if (write_speckles_file && speckles_img.isNull()) {
      // Even if despeckling didn't actually take place, we still need
      // to write an empty speckles file.  Making it a special case
      // is simply not worth it.
      BinaryImage(out_img.size(), WHITE).swap(speckles_img);
    }

    if (write_automask) {
      // Note that QDir::mkdir() will fail if the parent directory,
      // that is $OUT/cache doesn't exist. We want that behaviour,
      // as otherwise when loading a project from a different machine,
      // a whole bunch of bogus directories would be created.
      QDir().mkdir(automask_dir);
      // Also note that QDir::mkdir() will fail if the directory already exists,
      // so we ignore its return value here.
      if (!TiffWriter::writeImage(automask_file_path, automask_img.toQImage())) {
        invalidate_params = true;
      }
    }
    if (write_speckles_file) {
      if (!QDir().mkpath(speckles_dir)) {
        invalidate_params = true;
      } else if (!TiffWriter::writeImage(speckles_file_path, speckles_img.toQImage())) {
        invalidate_params = true;
      }
    }

    if (invalidate_params) {
      m_ptrSettings->removeOutputParams(m_pageId);
    } else {
      // Note that we can't reuse *_file_info objects
      // as we've just overwritten those files.
      const OutputParams out_params(
          new_output_image_params, OutputFileParams(QFileInfo(out_file_path)),
          render_params.splitOutput() ? OutputFileParams(QFileInfo(foreground_file_path)) : OutputFileParams(),
          render_params.splitOutput() ? OutputFileParams(QFileInfo(background_file_path)) : OutputFileParams(),
          render_params.originalBackground() ? OutputFileParams(QFileInfo(original_background_file_path))
                                             : OutputFileParams(),
          write_automask ? OutputFileParams(QFileInfo(automask_file_path)) : OutputFileParams(),
          write_speckles_file ? OutputFileParams(QFileInfo(speckles_file_path)) : OutputFileParams(), new_picture_zones,
          new_fill_zones);

      m_ptrSettings->setOutputParams(m_pageId, out_params);
    }

    m_ptrThumbnailCache->recreateThumbnail(ImageId(out_file_path), out_img);
  }

  const DespeckleState despeckle_state(out_img, speckles_img, params.despeckleLevel(), params.outputDpi());

  DespeckleVisualization despeckle_visualization;
  if (m_lastTab == TAB_DESPECKLING) {
    // Because constructing DespeckleVisualization takes a noticeable
    // amount of time, we only do it if we are sure we'll need it.
    // Otherwise it will get constructed on demand.
    despeckle_visualization = despeckle_state.visualize();
  }

  if (CommandLine::get().isGui()) {
    return make_intrusive<UiUpdater>(m_ptrFilter, m_ptrSettings, std::move(m_ptrDbg), params, new_xform,
                                     generator.getPostTransform(), generator.outputContentRect(), m_pageId,
                                     data.origImage(), out_img, automask_img, despeckle_state, despeckle_visualization,
                                     m_batchProcessing, m_debug);
  } else {
    return nullptr;
  }
}  // Task::process

/**
 * Delete output files mutually exclusive to m_pageId.
 */
void Task::deleteMutuallyExclusiveOutputFiles() {
  switch (m_pageId.subPage()) {
    case PageId::SINGLE_PAGE:
      QFile::remove(m_outFileNameGen.filePathFor(PageId(m_pageId.imageId(), PageId::LEFT_PAGE)));
      QFile::remove(m_outFileNameGen.filePathFor(PageId(m_pageId.imageId(), PageId::RIGHT_PAGE)));
      break;
    case PageId::LEFT_PAGE:
    case PageId::RIGHT_PAGE:
      QFile::remove(m_outFileNameGen.filePathFor(PageId(m_pageId.imageId(), PageId::SINGLE_PAGE)));
      break;
  }
}

/*============================ Task::UiUpdater ==========================*/

Task::UiUpdater::UiUpdater(intrusive_ptr<Filter> filter,
                           intrusive_ptr<Settings> settings,
                           std::unique_ptr<DebugImages> dbg_img,
                           const Params& params,
                           const ImageTransformation& xform,
                           const QTransform& postTransform,
                           const QRect& virt_content_rect,
                           const PageId& page_id,
                           const QImage& orig_image,
                           const QImage& output_image,
                           const BinaryImage& picture_mask,
                           const DespeckleState& despeckle_state,
                           const DespeckleVisualization& despeckle_visualization,
                           const bool batch,
                           const bool debug)
    : m_ptrFilter(std::move(filter)),
      m_ptrSettings(std::move(settings)),
      m_ptrDbg(std::move(dbg_img)),
      m_params(params),
      m_xform(xform),
      m_outputPostTransform(postTransform),
      m_virtContentRect(virt_content_rect),
      m_pageId(page_id),
      m_origImage(orig_image),
      m_downscaledOrigImage(ImageView::createDownscaledImage(orig_image)),
      m_outputImage(output_image),
      m_downscaledOutputImage(ImageView::createDownscaledImage(output_image)),
      m_pictureMask(picture_mask),
      m_despeckleState(despeckle_state),
      m_despeckleVisualization(despeckle_visualization),
      m_batchProcessing(batch),
      m_debug(debug) {}

void Task::UiUpdater::updateUI(FilterUiInterface* ui) {
  // This function is executed from the GUI thread.
  OptionsWidget* const opt_widget = m_ptrFilter->optionsWidget();
  opt_widget->postUpdateUI();
  ui->setOptionsWidget(opt_widget, ui->KEEP_OWNERSHIP);

  ui->invalidateThumbnail(m_pageId);

  if (m_batchProcessing) {
    return;
  }

  auto tab_image_rect_map = std::make_unique<std::unordered_map<ImageViewTab, QRectF, std::hash<int>>>();

  std::unique_ptr<ImageViewBase> image_view(new ImageView(m_outputImage, m_downscaledOutputImage));
  const QPixmap downscaled_output_pixmap(image_view->downscaledPixmap());
  tab_image_rect_map->insert(std::pair<ImageViewTab, QRectF>(TAB_OUTPUT, m_xform.resultingRect()));

  std::unique_ptr<ImageViewBase> dewarping_view(new DewarpingView(
      m_origImage, m_downscaledOrigImage, m_xform.transform(),
      PolygonUtils::convexHull(m_xform.resultingPreCropArea().toStdVector()), m_virtContentRect, m_pageId,
      m_params.dewarpingOptions(), m_params.distortionModel(), opt_widget->depthPerception()));
  const QPixmap downscaled_orig_pixmap(dewarping_view->downscaledPixmap());
  QObject::connect(opt_widget, SIGNAL(depthPerceptionChanged(double)), dewarping_view.get(),
                   SLOT(depthPerceptionChanged(double)));
  QObject::connect(dewarping_view.get(), SIGNAL(distortionModelChanged(const dewarping::DistortionModel&)), opt_widget,
                   SLOT(distortionModelChanged(const dewarping::DistortionModel&)));
  tab_image_rect_map->insert(
      std::pair<ImageViewTab, QRectF>(TAB_DEWARPING, m_xform.resultingPreCropArea().boundingRect()));

  std::unique_ptr<QWidget> picture_zone_editor;
  if (m_pictureMask.isNull()) {
    picture_zone_editor = std::make_unique<ErrorWidget>(tr("Picture zones are only available in Mixed mode."));
  } else {
    picture_zone_editor = std::make_unique<output::PictureZoneEditor>(
        m_origImage, downscaled_orig_pixmap, m_pictureMask, m_xform.transform(), m_xform.resultingPreCropArea(),
        m_pageId, m_ptrSettings);
    QObject::connect(picture_zone_editor.get(), SIGNAL(invalidateThumbnail(const PageId&)), opt_widget,
                     SIGNAL(invalidateThumbnail(const PageId&)));
    tab_image_rect_map->insert(
        std::pair<ImageViewTab, QRectF>(TAB_PICTURE_ZONES, m_xform.resultingPreCropArea().boundingRect()));
  }

  // We make sure we never need to update the original <-> output
  // mapping at run time, that is without reloading.
  // In OptionsWidget::dewarpingChanged() we make sure to reload
  // if we are on the "Fill Zones" tab, and if not, it will be reloaded
  // anyway when another tab is selected.
  boost::function<QPointF(const QPointF&)> orig_to_output;
  boost::function<QPointF(const QPointF&)> output_to_orig;
  if ((m_params.dewarpingOptions().dewarpingMode() != OFF) && m_params.distortionModel().isValid()) {
    std::shared_ptr<DewarpingPointMapper> mapper(
        new DewarpingPointMapper(m_params.distortionModel(), m_params.depthPerception().value(), m_xform.transform(),
                                 m_virtContentRect, m_outputPostTransform));
    orig_to_output = boost::bind(&DewarpingPointMapper::mapToDewarpedSpace, mapper, _1);
    output_to_orig = boost::bind(&DewarpingPointMapper::mapToWarpedSpace, mapper, _1);
  } else {
    typedef QPointF (QTransform::*MapPointFunc)(const QPointF&) const;
    orig_to_output = boost::bind((MapPointFunc) &QTransform::map, m_xform.transform(), _1);
    output_to_orig = boost::bind((MapPointFunc) &QTransform::map, m_xform.transformBack(), _1);
  }

  std::unique_ptr<QWidget> fill_zone_editor(new FillZoneEditor(m_outputImage, downscaled_output_pixmap, orig_to_output,
                                                               output_to_orig, m_pageId, m_ptrSettings));
  QObject::connect(fill_zone_editor.get(), SIGNAL(invalidateThumbnail(const PageId&)), opt_widget,
                   SIGNAL(invalidateThumbnail(const PageId&)));
  tab_image_rect_map->insert(std::pair<ImageViewTab, QRectF>(TAB_FILL_ZONES, m_xform.resultingRect()));

  std::unique_ptr<QWidget> despeckle_view;
  if (m_params.colorParams().colorMode() == COLOR_GRAYSCALE) {
    despeckle_view = std::make_unique<ErrorWidget>(tr("Despeckling can't be done in Color / Grayscale mode."));
  } else {
    despeckle_view = std::make_unique<output::DespeckleView>(m_despeckleState, m_despeckleVisualization, m_debug);
    QObject::connect(opt_widget, SIGNAL(despeckleLevelChanged(double, bool*)), despeckle_view.get(),
                     SLOT(despeckleLevelChanged(double, bool*)));
    tab_image_rect_map->insert(std::pair<ImageViewTab, QRectF>(TAB_DESPECKLING, m_xform.resultingRect()));
  }

  std::unique_ptr<TabbedImageView> tab_widget(new TabbedImageView);
  tab_widget->setDocumentMode(true);
  tab_widget->setTabPosition(QTabWidget::East);
  tab_widget->addTab(image_view.release(), tr("Output"), TAB_OUTPUT);
  tab_widget->addTab(picture_zone_editor.release(), tr("Picture Zones"), TAB_PICTURE_ZONES);
  tab_widget->addTab(fill_zone_editor.release(), tr("Fill Zones"), TAB_FILL_ZONES);
  tab_widget->addTab(dewarping_view.release(), tr("Dewarping"), TAB_DEWARPING);
  tab_widget->addTab(despeckle_view.release(), tr("Despeckling"), TAB_DESPECKLING);
  tab_widget->setCurrentTab(opt_widget->lastTab());
  tab_widget->setImageRectMap(std::move(tab_image_rect_map));

  QObject::connect(tab_widget.get(), SIGNAL(tabChanged(ImageViewTab)), opt_widget, SLOT(tabChanged(ImageViewTab)));

  ui->setImageWidget(tab_widget.release(), ui->TRANSFER_OWNERSHIP, m_ptrDbg.get());
}  // Task::UiUpdater::updateUI
}  // namespace output