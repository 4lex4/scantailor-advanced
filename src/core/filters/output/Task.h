// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef OUTPUT_TASK_H_
#define OUTPUT_TASK_H_

#include <QColor>
#include <QImage>
#include <memory>
#include "FilterResult.h"
#include "ImageViewTab.h"
#include "NonCopyable.h"
#include "OutputFileNameGenerator.h"
#include "PageId.h"
#include "ref_countable.h"

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

class Task : public ref_countable {
  DECLARE_NON_COPYABLE(Task)

 public:
  Task(intrusive_ptr<Filter> filter,
       intrusive_ptr<Settings> settings,
       intrusive_ptr<ThumbnailPixmapCache> thumbnail_cache,
       const PageId& page_id,
       const OutputFileNameGenerator& out_file_name_gen,
       ImageViewTab last_tab,
       bool batch,
       bool debug);

  ~Task() override;

  FilterResultPtr process(const TaskStatus& status, const FilterData& data, const QPolygonF& content_rect_phys);

 private:
  class UiUpdater;

  void deleteMutuallyExclusiveOutputFiles();

  intrusive_ptr<Filter> m_filter;
  intrusive_ptr<Settings> m_settings;
  intrusive_ptr<ThumbnailPixmapCache> m_thumbnailCache;
  std::unique_ptr<DebugImages> m_dbg;
  PageId m_pageId;
  OutputFileNameGenerator m_outFileNameGen;
  ImageViewTab m_lastTab;
  bool m_batchProcessing;
  bool m_debug;
};
}  // namespace output
#endif  // ifndef OUTPUT_TASK_H_
