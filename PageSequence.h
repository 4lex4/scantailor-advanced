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

#ifndef PAGE_SEQUENCE_H_
#define PAGE_SEQUENCE_H_

#include "PageInfo.h"
#include <vector>
#include <set>
#include <stddef.h>

class PageSequence
{
public:
    void append(PageInfo const& page_info);

    size_t numPages() const
    { return m_pages.size(); }

    PageInfo const& pageAt(size_t idx) const;

    std::set<PageId> selectAll() const;

    std::set<PageId> selectPagePlusFollowers(PageId const& page) const;

    std::set<PageId> selectEveryOther(PageId const& base) const;

private:
    std::vector<PageInfo> m_pages;
};

#endif
