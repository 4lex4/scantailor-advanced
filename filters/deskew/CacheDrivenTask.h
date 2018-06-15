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

#ifndef DESKEW_CACHEDRIVENTASK_H_
#define DESKEW_CACHEDRIVENTASK_H_

#include "NonCopyable.h"
#include "intrusive_ptr.h"
#include "ref_countable.h"

class QSizeF;
class PageInfo;
class AbstractFilterDataCollector;
class ImageTransformation;

namespace select_content {
class CacheDrivenTask;
}

namespace deskew {
class Settings;

class CacheDrivenTask : public ref_countable {
  DECLARE_NON_COPYABLE(CacheDrivenTask)

 public:
  CacheDrivenTask(intrusive_ptr<Settings> settings, intrusive_ptr<select_content::CacheDrivenTask> next_task);

  ~CacheDrivenTask() override;

  void process(const PageInfo& page_info, AbstractFilterDataCollector* collector, const ImageTransformation& xform);

 private:
  intrusive_ptr<select_content::CacheDrivenTask> m_ptrNextTask;
  intrusive_ptr<Settings> m_ptrSettings;
};
}  // namespace deskew
#endif  // ifndef DESKEW_CACHEDRIVENTASK_H_
