// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "PageOrientationPropagator.h"

#include <utility>
#include "CompositeCacheDrivenTask.h"
#include "OrthogonalRotation.h"
#include "PageOrientationCollector.h"
#include "PageSequence.h"
#include "ProjectPages.h"
#include "filters/page_split/Filter.h"

class PageOrientationPropagator::Collector : public PageOrientationCollector {
 public:
  void process(const OrthogonalRotation& orientation) override { m_orientation = orientation; }

  const OrthogonalRotation& orientation() const { return m_orientation; }

 private:
  OrthogonalRotation m_orientation;
};


PageOrientationPropagator::PageOrientationPropagator(intrusive_ptr<page_split::Filter> pageSplitFilter,
                                                     intrusive_ptr<CompositeCacheDrivenTask> task)
    : m_pageSplitFilter(std::move(pageSplitFilter)), m_task(std::move(task)) {}

PageOrientationPropagator::~PageOrientationPropagator() = default;

void PageOrientationPropagator::propagate(const ProjectPages& pages) {
  const PageSequence sequence(pages.toPageSequence(PAGE_VIEW));

  for (const PageInfo& pageInfo : sequence) {
    Collector collector;
    m_task->process(pageInfo, &collector);
    m_pageSplitFilter->pageOrientationUpdate(pageInfo.imageId(), collector.orientation());
  }
}
