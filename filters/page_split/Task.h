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

#ifndef PAGE_SPLIT_TASK_H_
#define PAGE_SPLIT_TASK_H_

#include <memory>
#include "FilterResult.h"
#include "NonCopyable.h"
#include "PageInfo.h"
#include "intrusive_ptr.h"
#include "ref_countable.h"

class TaskStatus;
class FilterData;
class DebugImages;
class ProjectPages;
class QImage;

namespace deskew {
class Task;
}

namespace page_split {
class Filter;
class Settings;

class PageLayout;

class Task : public ref_countable {
  DECLARE_NON_COPYABLE(Task)

 public:
  Task(intrusive_ptr<Filter> filter,
       intrusive_ptr<Settings> settings,
       intrusive_ptr<ProjectPages> pages,
       intrusive_ptr<deskew::Task> next_task,
       const PageInfo& page_info,
       bool batch_processing,
       bool debug);

  ~Task() override;

  FilterResultPtr process(const TaskStatus& status, const FilterData& data);

 private:
  class UiUpdater;

  intrusive_ptr<Filter> m_ptrFilter;
  intrusive_ptr<Settings> m_ptrSettings;
  intrusive_ptr<ProjectPages> m_ptrPages;
  intrusive_ptr<deskew::Task> m_ptrNextTask;
  std::unique_ptr<DebugImages> m_ptrDbg;
  PageInfo m_pageInfo;
  bool m_batchProcessing;
};
}  // namespace page_split
#endif  // ifndef PAGE_SPLIT_TASK_H_
