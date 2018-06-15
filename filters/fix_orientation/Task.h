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

  intrusive_ptr<Filter> m_ptrFilter;
  intrusive_ptr<page_split::Task> m_ptrNextTask;  // if null, this task is the final one
  intrusive_ptr<Settings> m_ptrSettings;
  intrusive_ptr<ImageSettings> m_ptrImageSettings;
  PageId m_pageId;
  ImageId m_imageId;
  bool m_batchProcessing;
};
}  // namespace fix_orientation
#endif  // ifndef FIX_ORIENTATION_TASK_H_
