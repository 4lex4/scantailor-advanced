/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2007-2008  Joseph Artsimovich <joseph_a@mail.ru>

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

#ifndef PAGE_LAYOUT_TASK_H_
#define PAGE_LAYOUT_TASK_H_

#include <QPolygonF>
#include "FilterResult.h"
#include "NonCopyable.h"
#include "PageId.h"
#include "ref_countable.h"

class TaskStatus;
class FilterData;
class ImageTransformation;
class QRectF;
class Dpi;

namespace output {
class Task;
}

namespace page_layout {
class Filter;
class Settings;

class Task : public ref_countable {
  DECLARE_NON_COPYABLE(Task)

 public:
  Task(intrusive_ptr<Filter> filter,
       intrusive_ptr<output::Task> next_task,
       intrusive_ptr<Settings> settings,
       const PageId& page_id,
       bool batch,
       bool debug);

  ~Task() override;

  FilterResultPtr process(const TaskStatus& status,
                          const FilterData& data,
                          const QRectF& page_rect,
                          const QRectF& content_rect);

 private:
  class UiUpdater;

  static QPolygonF shiftToRoundedOrigin(const QPolygonF& poly);

  intrusive_ptr<Filter> m_ptrFilter;
  intrusive_ptr<output::Task> m_ptrNextTask;
  intrusive_ptr<Settings> m_ptrSettings;
  PageId m_pageId;
  bool m_batchProcessing;
};
}  // namespace page_layout
#endif  // ifndef PAGE_LAYOUT_TASK_H_
