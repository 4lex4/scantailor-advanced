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

#ifndef FIX_ORIENTATION_CACHEDRIVENTASK_H_
#define FIX_ORIENTATION_CACHEDRIVENTASK_H_

#include "CompositeCacheDrivenTask.h"
#include "NonCopyable.h"
#include "intrusive_ptr.h"

class PageInfo;
class AbstractFilterDataCollector;

namespace page_split {
class CacheDrivenTask;
}

namespace fix_orientation {
class Settings;

class CacheDrivenTask : public CompositeCacheDrivenTask {
  DECLARE_NON_COPYABLE(CacheDrivenTask)

 public:
  CacheDrivenTask(intrusive_ptr<Settings> settings, intrusive_ptr<page_split::CacheDrivenTask> next_task);

  ~CacheDrivenTask() override;

  void process(const PageInfo& page_info, AbstractFilterDataCollector* collector) override;

 private:
  intrusive_ptr<page_split::CacheDrivenTask> m_ptrNextTask;
  intrusive_ptr<Settings> m_ptrSettings;
};
}  // namespace fix_orientation
#endif  // ifndef FIX_ORIENTATION_CACHEDRIVENTASK_H_
