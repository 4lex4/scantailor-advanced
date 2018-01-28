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

#ifndef STAGESEQUENCE_H_
#define STAGESEQUENCE_H_

#include "NonCopyable.h"
#include "ref_countable.h"
#include "intrusive_ptr.h"
#include "AbstractFilter.h"
#include "filters/fix_orientation/Filter.h"
#include "filters/page_split/Filter.h"
#include "filters/deskew/Filter.h"
#include "filters/select_content/Filter.h"
#include "filters/page_layout/Filter.h"
#include "filters/output/Filter.h"
#include <vector>

class PageId;
class ProjectPages;
class PageSelectionAccessor;
class AbstractRelinker;

class StageSequence : public ref_countable {
DECLARE_NON_COPYABLE(StageSequence)

public:
    typedef intrusive_ptr<AbstractFilter> FilterPtr;

    StageSequence(intrusive_ptr<ProjectPages> const& pages, PageSelectionAccessor const& page_selection_accessor);

    void performRelinking(AbstractRelinker const& relinker);

    std::vector<FilterPtr> const& filters() const {
        return m_filters;
    }

    int count() const {
        return static_cast<int>(m_filters.size());
    }

    FilterPtr const& filterAt(int idx) const {
        return m_filters[idx];
    }

    int findFilter(FilterPtr const& filter) const;

    intrusive_ptr<fix_orientation::Filter> const& fixOrientationFilter() const {
        return m_ptrFixOrientationFilter;
    }

    intrusive_ptr<page_split::Filter> const& pageSplitFilter() const {
        return m_ptrPageSplitFilter;
    }

    intrusive_ptr<deskew::Filter> const& deskewFilter() const {
        return m_ptrDeskewFilter;
    }

    intrusive_ptr<select_content::Filter> const& selectContentFilter() const {
        return m_ptrSelectContentFilter;
    }

    intrusive_ptr<page_layout::Filter> const& pageLayoutFilter() const {
        return m_ptrPageLayoutFilter;
    }

    intrusive_ptr<output::Filter> const& outputFilter() const {
        return m_ptrOutputFilter;
    }

    int fixOrientationFilterIdx() const {
        return m_fixOrientationFilterIdx;
    }

    int pageSplitFilterIdx() const {
        return m_pageSplitFilterIdx;
    }

    int deskewFilterIdx() const {
        return m_deskewFilterIdx;
    }

    int selectContentFilterIdx() const {
        return m_selectContentFilterIdx;
    }

    int pageLayoutFilterIdx() const {
        return m_pageLayoutFilterIdx;
    }

    int outputFilterIdx() const {
        return m_outputFilterIdx;
    }

private:
    intrusive_ptr<fix_orientation::Filter> m_ptrFixOrientationFilter;
    intrusive_ptr<page_split::Filter> m_ptrPageSplitFilter;
    intrusive_ptr<deskew::Filter> m_ptrDeskewFilter;
    intrusive_ptr<select_content::Filter> m_ptrSelectContentFilter;
    intrusive_ptr<page_layout::Filter> m_ptrPageLayoutFilter;
    intrusive_ptr<output::Filter> m_ptrOutputFilter;
    std::vector<FilterPtr> m_filters;
    int m_fixOrientationFilterIdx;
    int m_pageSplitFilterIdx;
    int m_deskewFilterIdx;
    int m_selectContentFilterIdx;
    int m_pageLayoutFilterIdx;
    int m_outputFilterIdx;
};


#endif  // ifndef STAGESEQUENCE_H_
