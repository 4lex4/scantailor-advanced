// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_CORE_PAGESEQUENCE_H_
#define SCANTAILOR_CORE_PAGESEQUENCE_H_

#include <cstddef>
#include <set>
#include <vector>

#include "PageInfo.h"

class PageSequence {
  // Member-wise copying is OK.
 public:
  void append(const PageInfo& pageInfo);

  size_t numPages() const;

  const PageInfo& pageAt(PageId page) const;

  const PageInfo& pageAt(size_t idx) const;

  int pageNo(const PageId& page) const;

  std::set<PageId> selectAll() const;

  std::set<PageId> selectPagePlusFollowers(const PageId& page) const;

  std::set<PageId> selectEveryOther(const PageId& base) const;

  std::vector<PageInfo>::iterator begin();

  std::vector<PageInfo>::iterator end();

  std::vector<PageInfo>::const_iterator begin() const;

  std::vector<PageInfo>::const_iterator end() const;

 private:
  std::vector<PageInfo> m_pages;
};


#endif
