// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "PageSequence.h"

void PageSequence::append(const PageInfo& pageInfo) {
  m_pages.push_back(pageInfo);
}

size_t PageSequence::numPages() const { return m_pages.size(); }

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

  for (const PageInfo& pageInfo : m_pages) {
    selection.insert(pageInfo.id());
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

  const int baseIdx = static_cast<int>(it - m_pages.begin());
  int idx = 0;
  for (const PageInfo& pageInfo : m_pages) {
    if (((idx - baseIdx) & 1) == 0) {
      selection.insert(pageInfo.id());
    }
    ++idx;
  }
  return selection;
}

std::vector<PageInfo>::iterator PageSequence::begin() { return m_pages.begin(); }

std::vector<PageInfo>::iterator PageSequence::end() { return m_pages.end(); }

std::vector<PageInfo>::const_iterator PageSequence::begin() const { return m_pages.cbegin(); }

std::vector<PageInfo>::const_iterator PageSequence::end() const { return m_pages.cend(); }
