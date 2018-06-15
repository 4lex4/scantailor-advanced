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

#include "ContentBoxPropagator.h"

#include <utility>
#include "CompositeCacheDrivenTask.h"
#include "ImageTransformation.h"
#include "PageSequence.h"
#include "ProjectPages.h"
#include "filter_dc/ContentBoxCollector.h"
#include "filters/page_layout/Filter.h"

class ContentBoxPropagator::Collector : public ContentBoxCollector {
 public:
  Collector();

  void process(const ImageTransformation& xform, const QRectF& content_rect) override;

  bool collected() const { return m_collected; }

  const ImageTransformation& xform() const { return m_xform; }

  const QRectF& contentRect() const { return m_contentRect; }

 private:
  ImageTransformation m_xform;
  QRectF m_contentRect;
  bool m_collected;
};


ContentBoxPropagator::ContentBoxPropagator(intrusive_ptr<page_layout::Filter> page_layout_filter,
                                           intrusive_ptr<CompositeCacheDrivenTask> task)
    : m_ptrPageLayoutFilter(std::move(page_layout_filter)), m_ptrTask(std::move(task)) {}

ContentBoxPropagator::~ContentBoxPropagator() = default;

void ContentBoxPropagator::propagate(const ProjectPages& pages) {
  const PageSequence sequence(pages.toPageSequence(PAGE_VIEW));

  for (const PageInfo& page_info : sequence) {
    Collector collector;
    m_ptrTask->process(page_info, &collector);
    if (collector.collected()) {
      m_ptrPageLayoutFilter->setContentBox(page_info.id(), collector.xform(), collector.contentRect());
    } else {
      m_ptrPageLayoutFilter->invalidateContentBox(page_info.id());
    }
  }
}

/*=================== ContentBoxPropagator::Collector ====================*/

ContentBoxPropagator::Collector::Collector() : m_xform(QRectF(0, 0, 1, 1), Dpi(300, 300)), m_collected(false) {}

void ContentBoxPropagator::Collector::process(const ImageTransformation& xform, const QRectF& content_rect) {
  m_xform = xform;
  m_contentRect = content_rect;
  m_collected = true;
}
