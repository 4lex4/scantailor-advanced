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

#ifndef PAGE_SPLIT_CACHEDRIVENTASK_H_
#define PAGE_SPLIT_CACHEDRIVENTASK_H_

#include "NonCopyable.h"
#include "intrusive_ptr.h"
#include "ref_countable.h"

class QSizeF;
class PageInfo;
class AbstractFilterDataCollector;
class ImageTransformation;
class ProjectPages;

namespace deskew {
class CacheDrivenTask;
}

namespace page_split {
class Settings;

class CacheDrivenTask : public ref_countable {
  DECLARE_NON_COPYABLE(CacheDrivenTask)

 public:
  CacheDrivenTask(intrusive_ptr<Settings> settings,
                  intrusive_ptr<ProjectPages> projectPages,
                  intrusive_ptr<deskew::CacheDrivenTask> next_task);

  ~CacheDrivenTask() override;

  void process(const PageInfo& page_info, AbstractFilterDataCollector* collector, const ImageTransformation& xform);

 private:
  intrusive_ptr<deskew::CacheDrivenTask> m_ptrNextTask;
  intrusive_ptr<Settings> m_ptrSettings;
  intrusive_ptr<ProjectPages> m_projectPages;
};
}  // namespace page_split
#endif  // ifndef PAGE_SPLIT_CACHEDRIVENTASK_H_
