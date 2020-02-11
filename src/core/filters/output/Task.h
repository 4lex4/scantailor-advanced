// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_OUTPUT_TASK_H_
#define SCANTAILOR_OUTPUT_TASK_H_

#include <QColor>
#include <QImage>
#include <memory>

#include "FilterResult.h"
#include "ImageViewTab.h"
#include "NonCopyable.h"
#include "OutputFileNameGenerator.h"
#include "PageId.h"

class DebugImages;
class TaskStatus;
class FilterData;
class ThumbnailPixmapCache;
class ImageTransformation;
class QPolygonF;
class QSize;
class QImage;
class Dpi;

namespace imageproc {
class BinaryImage;
}

namespace output {
class Filter;
class Settings;

class Task {
  DECLARE_NON_COPYABLE(Task)

 public:
  Task(std::shared_ptr<Filter> filter,
       std::shared_ptr<Settings> settings,
       std::shared_ptr<ThumbnailPixmapCache> thumbnailCache,
       const PageId& pageId,
       const OutputFileNameGenerator& outFileNameGen,
       ImageViewTab lastTab,
       bool batch,
       bool debug);

  virtual ~Task();

  FilterResultPtr process(const TaskStatus& status, const FilterData& data, const QPolygonF& contentRectPhys);

 private:
  class UiUpdater;

  void deleteMutuallyExclusiveOutputFiles();

  std::shared_ptr<Filter> m_filter;
  std::shared_ptr<Settings> m_settings;
  std::shared_ptr<ThumbnailPixmapCache> m_thumbnailCache;
  std::unique_ptr<DebugImages> m_dbg;
  PageId m_pageId;
  OutputFileNameGenerator m_outFileNameGen;
  ImageViewTab m_lastTab;
  bool m_batchProcessing;
  bool m_debug;
};
}  // namespace output
#endif  // ifndef SCANTAILOR_OUTPUT_TASK_H_
