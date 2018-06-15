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

#include "StageSequence.h"
#include "ProjectPages.h"

StageSequence::StageSequence(const intrusive_ptr<ProjectPages>& pages,
                             const PageSelectionAccessor& page_selection_accessor)
    : m_ptrFixOrientationFilter(new fix_orientation::Filter(page_selection_accessor)),
      m_ptrPageSplitFilter(new page_split::Filter(pages, page_selection_accessor)),
      m_ptrDeskewFilter(new deskew::Filter(page_selection_accessor)),
      m_ptrSelectContentFilter(new select_content::Filter(page_selection_accessor)),
      m_ptrPageLayoutFilter(new page_layout::Filter(pages, page_selection_accessor)),
      m_ptrOutputFilter(new output::Filter(page_selection_accessor)) {
  m_fixOrientationFilterIdx = static_cast<int>(m_filters.size());
  m_filters.emplace_back(m_ptrFixOrientationFilter);

  m_pageSplitFilterIdx = static_cast<int>(m_filters.size());
  m_filters.emplace_back(m_ptrPageSplitFilter);

  m_deskewFilterIdx = static_cast<int>(m_filters.size());
  m_filters.emplace_back(m_ptrDeskewFilter);

  m_selectContentFilterIdx = static_cast<int>(m_filters.size());
  m_filters.emplace_back(m_ptrSelectContentFilter);

  m_pageLayoutFilterIdx = static_cast<int>(m_filters.size());
  m_filters.emplace_back(m_ptrPageLayoutFilter);

  m_outputFilterIdx = static_cast<int>(m_filters.size());
  m_filters.emplace_back(m_ptrOutputFilter);
}

void StageSequence::performRelinking(const AbstractRelinker& relinker) {
  for (FilterPtr& filter : m_filters) {
    filter->performRelinking(relinker);
  }
}

int StageSequence::findFilter(const FilterPtr& filter) const {
  int idx = 0;
  for (const FilterPtr& f : m_filters) {
    if (f == filter) {
      return idx;
    }
    ++idx;
  }

  return -1;
}
