// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "StageSequence.h"
#include "ProjectPages.h"

StageSequence::StageSequence(const intrusive_ptr<ProjectPages>& pages,
                             const PageSelectionAccessor& page_selection_accessor)
    : m_fixOrientationFilter(new fix_orientation::Filter(page_selection_accessor)),
      m_pageSplitFilter(new page_split::Filter(pages, page_selection_accessor)),
      m_deskewFilter(new deskew::Filter(page_selection_accessor)),
      m_selectContentFilter(new select_content::Filter(page_selection_accessor)),
      m_pageLayoutFilter(new page_layout::Filter(pages, page_selection_accessor)),
      m_outputFilter(new output::Filter(page_selection_accessor)) {
  m_fixOrientationFilterIdx = static_cast<int>(m_filters.size());
  m_filters.emplace_back(m_fixOrientationFilter);

  m_pageSplitFilterIdx = static_cast<int>(m_filters.size());
  m_filters.emplace_back(m_pageSplitFilter);

  m_deskewFilterIdx = static_cast<int>(m_filters.size());
  m_filters.emplace_back(m_deskewFilter);

  m_selectContentFilterIdx = static_cast<int>(m_filters.size());
  m_filters.emplace_back(m_selectContentFilter);

  m_pageLayoutFilterIdx = static_cast<int>(m_filters.size());
  m_filters.emplace_back(m_pageLayoutFilter);

  m_outputFilterIdx = static_cast<int>(m_filters.size());
  m_filters.emplace_back(m_outputFilter);
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
