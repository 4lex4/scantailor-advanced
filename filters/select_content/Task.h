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

#ifndef SELECT_CONTENT_TASK_H_
#define SELECT_CONTENT_TASK_H_

#include <QRectF>
#include <QSizeF>
#include <memory>
#include "FilterResult.h"
#include "NonCopyable.h"
#include "PageId.h"
#include "ref_countable.h"

class TaskStatus;
class FilterData;
class DebugImages;
class ImageTransformation;
class Dpi;

namespace page_layout {
class Task;
}

namespace select_content {
class Filter;
class Settings;

class Task : public ref_countable {
  DECLARE_NON_COPYABLE(Task)

 public:
  Task(intrusive_ptr<Filter> filter,
       intrusive_ptr<page_layout::Task> next_task,
       intrusive_ptr<Settings> settings,
       const PageId& page_id,
       bool batch,
       bool debug);

  ~Task() override;

  FilterResultPtr process(const TaskStatus& status, const FilterData& data);

 private:
  class UiUpdater;

  intrusive_ptr<Filter> m_ptrFilter;
  intrusive_ptr<page_layout::Task> m_ptrNextTask;
  intrusive_ptr<Settings> m_ptrSettings;
  std::unique_ptr<DebugImages> m_ptrDbg;
  PageId m_pageId;
  bool m_batchProcessing;
};
}  // namespace select_content
#endif  // ifndef SELECT_CONTENT_TASK_H_
