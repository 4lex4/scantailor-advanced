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

#ifndef OUTPUT_CACHEDRIVENTASK_H_
#define OUTPUT_CACHEDRIVENTASK_H_

#include "NonCopyable.h"
#include "OutputFileNameGenerator.h"
#include "intrusive_ptr.h"
#include "ref_countable.h"

class QPolygonF;
class PageInfo;
class AbstractFilterDataCollector;
class ImageTransformation;

namespace output {
class Settings;

class CacheDrivenTask : public ref_countable {
  DECLARE_NON_COPYABLE(CacheDrivenTask)

 public:
  CacheDrivenTask(intrusive_ptr<Settings> settings, const OutputFileNameGenerator& out_file_name_gen);

  ~CacheDrivenTask() override;

  void process(const PageInfo& page_info,
               AbstractFilterDataCollector* collector,
               const ImageTransformation& xform,
               const QPolygonF& content_rect_phys);

 private:
  intrusive_ptr<Settings> m_ptrSettings;
  OutputFileNameGenerator m_outFileNameGen;
};
}  // namespace output
#endif  // ifndef OUTPUT_CACHEDRIVENTASK_H_
