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

#include "PageSequence.h"

void PageSequence::append(const PageInfo& page_info) {
  m_pages.push_back(page_info);
}

const PageInfo& PageSequence::pageAt(const size_t idx) const {
  return m_pages.at(idx);  // may throw
}

const PageInfo& PageSequence::pageAt(const PageId page) const {
  auto it(m_pages.begin());
  const auto end(m_pages.end());
  for (; it != end && it->id() != page; ++it) {
  }
  return *it;
}

int PageSequence::pageNo(const PageId& page) const {
  auto it(m_pages.begin());
  const auto end(m_pages.end());
  int res = 0;
  for (; it != end && it->id() != page; ++it, ++res) {
  }
  return (it == end) ? -1 : res;
}

std::set<PageId> PageSequence::selectAll() const {
  std::set<PageId> selection;

  for (const PageInfo& page_info : m_pages) {
    selection.insert(page_info.id());
  }

  return selection;
}

std::set<PageId> PageSequence::selectPagePlusFollowers(const PageId& page) const {
  std::set<PageId> selection;

  auto it(m_pages.begin());
  const auto end(m_pages.end());
  for (; it != end && it->id() != page; ++it) {
    // Continue until we have a match.
  }
  for (; it != end; ++it) {
    selection.insert(it->id());
  }

  return selection;
}

std::set<PageId> PageSequence::selectEveryOther(const PageId& base) const {
  std::set<PageId> selection;

  auto it(m_pages.begin());
  const auto end(m_pages.end());
  for (; it != end && it->id() != base; ++it) {
    // Continue until we have a match.
  }
  if (it == end) {
    return selection;
  }

  const int base_idx = static_cast<const int>(it - m_pages.begin());
  int idx = 0;
  for (const PageInfo& page_info : m_pages) {
    if (((idx - base_idx) & 1) == 0) {
      selection.insert(page_info.id());
    }
    ++idx;
  }

  return selection;
}
