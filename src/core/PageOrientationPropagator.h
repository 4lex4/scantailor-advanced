// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_CORE_PAGEORIENTATIONPROPAGATOR_H_
#define SCANTAILOR_CORE_PAGEORIENTATIONPROPAGATOR_H_

#include <QSizeF>
#include <memory>

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
  PageOrientationPropagator(std::shared_ptr<page_split::Filter> pageSplitFilter,
                            std::shared_ptr<CompositeCacheDrivenTask> task);

  ~PageOrientationPropagator();

  void propagate(const ProjectPages& pages);

 private:
  class Collector;

  std::shared_ptr<page_split::Filter> m_pageSplitFilter;
  std::shared_ptr<CompositeCacheDrivenTask> m_task;
};


#endif  // ifndef SCANTAILOR_CORE_PAGEORIENTATIONPROPAGATOR_H_
