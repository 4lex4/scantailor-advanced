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

#ifndef CONTENTBOXPROPAGATOR_H_
#define CONTENTBOXPROPAGATOR_H_

#include <QSizeF>
#include "intrusive_ptr.h"

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
  ContentBoxPropagator(intrusive_ptr<page_layout::Filter> page_layout_filter,
                       intrusive_ptr<CompositeCacheDrivenTask> task);

  ~ContentBoxPropagator();

  void propagate(const ProjectPages& pages);

 private:
  class Collector;

  intrusive_ptr<page_layout::Filter> m_ptrPageLayoutFilter;
  intrusive_ptr<CompositeCacheDrivenTask> m_ptrTask;
};


#endif  // ifndef CONTENTBOXPROPAGATOR_H_
