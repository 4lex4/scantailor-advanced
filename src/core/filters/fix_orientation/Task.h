// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_FIX_ORIENTATION_TASK_H_
#define SCANTAILOR_FIX_ORIENTATION_TASK_H_

#include <FilterData.h>

#include <memory>

#include "FilterResult.h"
#include "ImageId.h"
#include "NonCopyable.h"

class TaskStatus;
class QImage;

namespace page_split {
class Task;
}

namespace fix_orientation {
class Filter;
class Settings;

class Task {
  DECLARE_NON_COPYABLE(Task)

 public:
  Task(const PageId& pageId,
       std::shared_ptr<Filter> filter,
       std::shared_ptr<Settings> settings,
       std::shared_ptr<ImageSettings> imageSettings,
       std::shared_ptr<page_split::Task> nextTask,
       bool batchProcessing);

  virtual ~Task();

  FilterResultPtr process(const TaskStatus& status, FilterData data);

 private:
  class UiUpdater;

  void updateFilterData(FilterData& data);

  std::shared_ptr<Filter> m_filter;
  std::shared_ptr<page_split::Task> m_nextTask;  // if null, this task is the final one
  std::shared_ptr<Settings> m_settings;
  std::shared_ptr<ImageSettings> m_imageSettings;
  PageId m_pageId;
  ImageId m_imageId;
  bool m_batchProcessing;
};
}  // namespace fix_orientation
#endif  // ifndef SCANTAILOR_FIX_ORIENTATION_TASK_H_
