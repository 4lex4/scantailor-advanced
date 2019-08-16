// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

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

  intrusive_ptr<Filter> m_filter;
  intrusive_ptr<page_layout::Task> m_nextTask;
  intrusive_ptr<Settings> m_settings;
  std::unique_ptr<DebugImages> m_dbg;
  PageId m_pageId;
  bool m_batchProcessing;
};
}  // namespace select_content
#endif  // ifndef SELECT_CONTENT_TASK_H_
