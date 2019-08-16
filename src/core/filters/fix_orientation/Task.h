// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef FIX_ORIENTATION_TASK_H_
#define FIX_ORIENTATION_TASK_H_

#include <FilterData.h>
#include "FilterResult.h"
#include "ImageId.h"
#include "NonCopyable.h"
#include "intrusive_ptr.h"
#include "ref_countable.h"

class TaskStatus;
class QImage;

namespace page_split {
class Task;
}

namespace fix_orientation {
class Filter;
class Settings;

class Task : public ref_countable {
  DECLARE_NON_COPYABLE(Task)

 public:
  Task(const PageId& page_id,
       intrusive_ptr<Filter> filter,
       intrusive_ptr<Settings> settings,
       intrusive_ptr<ImageSettings> image_settings,
       intrusive_ptr<page_split::Task> next_task,
       bool batch_processing);

  ~Task() override;

  FilterResultPtr process(const TaskStatus& status, FilterData data);

 private:
  class UiUpdater;

  void updateFilterData(FilterData& data);

  intrusive_ptr<Filter> m_filter;
  intrusive_ptr<page_split::Task> m_nextTask;  // if null, this task is the final one
  intrusive_ptr<Settings> m_settings;
  intrusive_ptr<ImageSettings> m_imageSettings;
  PageId m_pageId;
  ImageId m_imageId;
  bool m_batchProcessing;
};
}  // namespace fix_orientation
#endif  // ifndef FIX_ORIENTATION_TASK_H_
