// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_CORE_CONTENTBOXPROPAGATOR_H_
#define SCANTAILOR_CORE_CONTENTBOXPROPAGATOR_H_

#include <QSizeF>
#include <memory>

class CompositeCacheDrivenTask;
class ProjectPages;

namespace page_layout {
class Filter;
}

/**
 * \brief Propagates content boxes from "Select Content" to "Margins" stage.
 *
 * This is necessary in the following case:\n
 * You go back from "Margins" to one of the previous stages and make
 * adjustments there to several pages.  Now you return to "Margins" and
 * expect to see the results of all your adjustments (not just the current page)
 * there.
 */
class ContentBoxPropagator {
 public:
  ContentBoxPropagator(std::shared_ptr<page_layout::Filter> pageLayoutFilter,
                       std::shared_ptr<CompositeCacheDrivenTask> task);

  ~ContentBoxPropagator();

  void propagate(const ProjectPages& pages);

 private:
  class Collector;

  std::shared_ptr<page_layout::Filter> m_pageLayoutFilter;
  std::shared_ptr<CompositeCacheDrivenTask> m_task;
};


#endif  // ifndef SCANTAILOR_CORE_CONTENTBOXPROPAGATOR_H_
