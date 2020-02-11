// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "ContentBoxPropagator.h"

#include <utility>

#include "CompositeCacheDrivenTask.h"
#include "ContentBoxCollector.h"
#include "ImageTransformation.h"
#include "PageSequence.h"
#include "ProjectPages.h"
#include "filters/page_layout/Filter.h"

class ContentBoxPropagator::Collector : public ContentBoxCollector {
 public:
  Collector();

  void process(const ImageTransformation& xform, const QRectF& contentRect) override;

  bool collected() const { return m_collected; }

  const ImageTransformation& xform() const { return m_xform; }

  const QRectF& contentRect() const { return m_contentRect; }

 private:
  ImageTransformation m_xform;
  QRectF m_contentRect;
  bool m_collected;
};


ContentBoxPropagator::ContentBoxPropagator(std::shared_ptr<page_layout::Filter> pageLayoutFilter,
                                           std::shared_ptr<CompositeCacheDrivenTask> task)
    : m_pageLayoutFilter(std::move(pageLayoutFilter)), m_task(std::move(task)) {}

ContentBoxPropagator::~ContentBoxPropagator() = default;

void ContentBoxPropagator::propagate(const ProjectPages& pages) {
  const PageSequence sequence(pages.toPageSequence(PAGE_VIEW));

  for (const PageInfo& pageInfo : sequence) {
    Collector collector;
    m_task->process(pageInfo, &collector);
    if (collector.collected()) {
      m_pageLayoutFilter->setContentBox(pageInfo.id(), collector.xform(), collector.contentRect());
    } else {
      m_pageLayoutFilter->invalidateContentBox(pageInfo.id());
    }
  }
}

/*=================== ContentBoxPropagator::Collector ====================*/

ContentBoxPropagator::Collector::Collector() : m_xform(QRectF(0, 0, 1, 1), Dpi(300, 300)), m_collected(false) {}

void ContentBoxPropagator::Collector::process(const ImageTransformation& xform, const QRectF& contentRect) {
  m_xform = xform;
  m_contentRect = contentRect;
  m_collected = true;
}
