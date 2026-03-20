// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_CORE_BATCHPIPELINE_H_
#define SCANTAILOR_CORE_BATCHPIPELINE_H_

#include <memory>

#include "BackgroundTask.h"
#include "PageInfo.h"

class StageSequence;
class ThumbnailPixmapCache;
class OutputFileNameGenerator;
class ProjectPages;

class BatchPipeline {
 public:
  static BackgroundTaskPtr createCompositeTask(const PageInfo& page,
                                               int lastFilterIdx,
                                               const StageSequence& stages,
                                               const std::shared_ptr<ThumbnailPixmapCache>& thumbnailCache,
                                               const OutputFileNameGenerator& outFileNameGen,
                                               const std::shared_ptr<ProjectPages>& pages,
                                               bool batch,
                                               bool debug);
};

#endif  // SCANTAILOR_CORE_BATCHPIPELINE_H_
