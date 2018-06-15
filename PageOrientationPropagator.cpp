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

#include "PageOrientationPropagator.h"

#include <utility>
#include "CompositeCacheDrivenTask.h"
#include "OrthogonalRotation.h"
#include "PageSequence.h"
#include "ProjectPages.h"
#include "filter_dc/PageOrientationCollector.h"
#include "filters/page_split/Filter.h"

class PageOrientationPropagator::Collector : public PageOrientationCollector {
 public:
  void process(const OrthogonalRotation& orientation) override { m_orientation = orientation; }

  const OrthogonalRotation& orientation() const { return m_orientation; }

 private:
  OrthogonalRotation m_orientation;
};


PageOrientationPropagator::PageOrientationPropagator(intrusive_ptr<page_split::Filter> page_split_filter,
                                                     intrusive_ptr<CompositeCacheDrivenTask> task)
    : m_ptrPageSplitFilter(std::move(page_split_filter)), m_ptrTask(std::move(task)) {}

PageOrientationPropagator::~PageOrientationPropagator() = default;

void PageOrientationPropagator::propagate(const ProjectPages& pages) {
  const PageSequence sequence(pages.toPageSequence(PAGE_VIEW));

  for (const PageInfo& page_info : sequence) {
    Collector collector;
    m_ptrTask->process(page_info, &collector);
    m_ptrPageSplitFilter->pageOrientationUpdate(page_info.imageId(), collector.orientation());
  }
}
