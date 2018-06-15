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

  intrusive_ptr<Filter> m_ptrFilter;
  intrusive_ptr<Settings> m_ptrSettings;
  intrusive_ptr<ThumbnailPixmapCache> m_ptrThumbnailCache;
  std::unique_ptr<DebugImages> m_ptrDbg;
  PageId m_pageId;
  OutputFileNameGenerator m_outFileNameGen;
  ImageViewTab m_lastTab;
  bool m_batchProcessing;
  bool m_debug;
};
}  // namespace output
#endif  // ifndef OUTPUT_TASK_H_
