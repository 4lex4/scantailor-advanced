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

#include <cstddef>
#include <set>
#include <vector>
#include "PageInfo.h"

class PageSequence {
  // Member-wise copying is OK.
 public:
  void append(const PageInfo& page_info);

  size_t numPages() const { return m_pages.size(); }

  const PageInfo& pageAt(PageId page) const;

  const PageInfo& pageAt(size_t idx) const;

  int pageNo(const PageId& page) const;

  std::set<PageId> selectAll() const;

  std::set<PageId> selectPagePlusFollowers(const PageId& page) const;

  std::set<PageId> selectEveryOther(const PageId& base) const;

  std::vector<PageInfo>::iterator begin() { return m_pages.begin(); }

  std::vector<PageInfo>::iterator end() { return m_pages.end(); }

  std::vector<PageInfo>::const_iterator begin() const { return m_pages.cbegin(); }

  std::vector<PageInfo>::const_iterator end() const { return m_pages.cend(); }

 private:
  std::vector<PageInfo> m_pages;
};


#endif
