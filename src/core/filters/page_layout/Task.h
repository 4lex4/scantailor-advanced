// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_PAGE_LAYOUT_TASK_H_
#define SCANTAILOR_PAGE_LAYOUT_TASK_H_

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
       intrusive_ptr<output::Task> nextTask,
       intrusive_ptr<Settings> settings,
       const PageId& pageId,
       bool batch,
       bool debug);

  ~Task() override;

  FilterResultPtr process(const TaskStatus& status,
                          const FilterData& data,
                          const QRectF& pageRect,
                          const QRectF& contentRect);

 private:
  class UiUpdater;

  intrusive_ptr<Filter> m_filter;
  intrusive_ptr<output::Task> m_nextTask;
  intrusive_ptr<Settings> m_settings;
  PageId m_pageId;
  bool m_batchProcessing;
};
}  // namespace page_layout
#endif  // ifndef SCANTAILOR_PAGE_LAYOUT_TASK_H_
