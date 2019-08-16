// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

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

  intrusive_ptr<Filter> m_filter;
  intrusive_ptr<Settings> m_settings;
  intrusive_ptr<ProjectPages> m_pages;
  intrusive_ptr<deskew::Task> m_nextTask;
  std::unique_ptr<DebugImages> m_dbg;
  PageInfo m_pageInfo;
  bool m_batchProcessing;
};
}  // namespace page_split
#endif  // ifndef PAGE_SPLIT_TASK_H_
