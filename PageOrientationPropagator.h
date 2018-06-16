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

#ifndef PAGE_ORIENTATION_PROPAGATOR_H_
#define PAGE_ORIENTATION_PROPAGATOR_H_

#include <QSizeF>
#include "intrusive_ptr.h"

class CompositeCacheDrivenTask;
class ProjectPages;

namespace page_split {
class Filter;
}

/**
 * \brief Propagates page orientations from the "Fix Orientation"
 *        to the "Split Pages" stage.
 *
 * This is necessary because the decision of whether to treat a scan
 * as two pages or one needs to be made collectively by the "Fix Orientation"
 * and "Split Pages" stages.  "Split Pages" might or might not know the definite
 * answer, while "Fix Orientation" provides a hint.
 */
class PageOrientationPropagator {
 public:
  PageOrientationPropagator(intrusive_ptr<page_split::Filter> page_split_filter,
                            intrusive_ptr<CompositeCacheDrivenTask> task);

  ~PageOrientationPropagator();

  void propagate(const ProjectPages& pages);

 private:
  class Collector;

  intrusive_ptr<page_split::Filter> m_ptrPageSplitFilter;
  intrusive_ptr<CompositeCacheDrivenTask> m_ptrTask;
};


#endif  // ifndef PAGE_ORIENTATION_PROPAGATOR_H_
