// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_CORE_STAGESEQUENCE_H_
#define SCANTAILOR_CORE_STAGESEQUENCE_H_

#include <vector>
#include "AbstractFilter.h"
#include "NonCopyable.h"
#include "filters/deskew/Filter.h"
#include "filters/fix_orientation/Filter.h"
#include "filters/output/Filter.h"
#include "filters/page_layout/Filter.h"
#include "filters/page_split/Filter.h"
#include "filters/select_content/Filter.h"
#include "intrusive_ptr.h"
#include "ref_countable.h"

class PageId;
class ProjectPages;
class PageSelectionAccessor;
class AbstractRelinker;

class StageSequence : public ref_countable {
  DECLARE_NON_COPYABLE(StageSequence)

 public:
  typedef intrusive_ptr<AbstractFilter> FilterPtr;

  StageSequence(const intrusive_ptr<ProjectPages>& pages, const PageSelectionAccessor& pageSelectionAccessor);

  void performRelinking(const AbstractRelinker& relinker);

  const std::vector<FilterPtr>& filters() const { return m_filters; }

  int count() const { return static_cast<int>(m_filters.size()); }

  const FilterPtr& filterAt(int idx) const { return m_filters[idx]; }

  int findFilter(const FilterPtr& filter) const;

  const intrusive_ptr<fix_orientation::Filter>& fixOrientationFilter() const { return m_fixOrientationFilter; }

  const intrusive_ptr<page_split::Filter>& pageSplitFilter() const { return m_pageSplitFilter; }

  const intrusive_ptr<deskew::Filter>& deskewFilter() const { return m_deskewFilter; }

  const intrusive_ptr<select_content::Filter>& selectContentFilter() const { return m_selectContentFilter; }

  const intrusive_ptr<page_layout::Filter>& pageLayoutFilter() const { return m_pageLayoutFilter; }

  const intrusive_ptr<output::Filter>& outputFilter() const { return m_outputFilter; }

  int fixOrientationFilterIdx() const { return m_fixOrientationFilterIdx; }

  int pageSplitFilterIdx() const { return m_pageSplitFilterIdx; }

  int deskewFilterIdx() const { return m_deskewFilterIdx; }

  int selectContentFilterIdx() const { return m_selectContentFilterIdx; }

  int pageLayoutFilterIdx() const { return m_pageLayoutFilterIdx; }

  int outputFilterIdx() const { return m_outputFilterIdx; }

 private:
  intrusive_ptr<fix_orientation::Filter> m_fixOrientationFilter;
  intrusive_ptr<page_split::Filter> m_pageSplitFilter;
  intrusive_ptr<deskew::Filter> m_deskewFilter;
  intrusive_ptr<select_content::Filter> m_selectContentFilter;
  intrusive_ptr<page_layout::Filter> m_pageLayoutFilter;
  intrusive_ptr<output::Filter> m_outputFilter;
  std::vector<FilterPtr> m_filters;
  int m_fixOrientationFilterIdx;
  int m_pageSplitFilterIdx;
  int m_deskewFilterIdx;
  int m_selectContentFilterIdx;
  int m_pageLayoutFilterIdx;
  int m_outputFilterIdx;
};


#endif  // ifndef SCANTAILOR_CORE_STAGESEQUENCE_H_
