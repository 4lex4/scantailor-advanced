// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "BatchPipeline.h"

#include "LoadFileTask.h"
#include "StageSequence.h"
#include "filters/deskew/Filter.h"
#include "filters/fix_orientation/Task.h"
#include "filters/output/Filter.h"
#include "filters/page_layout/Filter.h"
#include "filters/page_split/Filter.h"
#include "filters/select_content/Filter.h"

BackgroundTaskPtr BatchPipeline::createCompositeTask(const PageInfo& page,
                                                     int lastFilterIdx,
                                                     const StageSequence& stages,
                                                     const std::shared_ptr<ThumbnailPixmapCache>& thumbnailCache,
                                                     const OutputFileNameGenerator& outFileNameGen,
                                                     const std::shared_ptr<ProjectPages>& pages,
                                                     bool batch,
                                                     bool debug) {
  std::shared_ptr<fix_orientation::Task> fixOrientationTask;
  std::shared_ptr<page_split::Task> pageSplitTask;
  std::shared_ptr<deskew::Task> deskewTask;
  std::shared_ptr<select_content::Task> selectContentTask;
  std::shared_ptr<page_layout::Task> pageLayoutTask;
  std::shared_ptr<output::Task> outputTask;

  if (batch) {
    debug = false;
  }

  if (lastFilterIdx >= stages.outputFilterIdx()) {
    outputTask = stages.outputFilter()->createTask(page.id(), thumbnailCache, outFileNameGen, batch, debug);
    debug = false;
  }
  if (lastFilterIdx >= stages.pageLayoutFilterIdx()) {
    pageLayoutTask = stages.pageLayoutFilter()->createTask(page.id(), outputTask, batch, debug);
    debug = false;
  }
  if (lastFilterIdx >= stages.selectContentFilterIdx()) {
    selectContentTask = stages.selectContentFilter()->createTask(page.id(), pageLayoutTask, batch, debug);
    debug = false;
  }
  if (lastFilterIdx >= stages.deskewFilterIdx()) {
    deskewTask = stages.deskewFilter()->createTask(page.id(), selectContentTask, batch, debug);
    debug = false;
  }
  if (lastFilterIdx >= stages.pageSplitFilterIdx()) {
    pageSplitTask = stages.pageSplitFilter()->createTask(page, deskewTask, batch, debug);
    debug = false;
  }
  if (lastFilterIdx >= stages.fixOrientationFilterIdx()) {
    fixOrientationTask = stages.fixOrientationFilter()->createTask(page.id(), pageSplitTask, batch);
    debug = false;
  }
  assert(fixOrientationTask);
  return std::make_shared<LoadFileTask>(batch ? BackgroundTask::BATCH : BackgroundTask::INTERACTIVE, page,
                                        thumbnailCache, pages, fixOrientationTask);
}
