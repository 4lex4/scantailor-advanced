// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_CORE_PAGERANGE_H_
#define SCANTAILOR_CORE_PAGERANGE_H_

#include <set>
#include <vector>
#include "PageId.h"

class PageRange {
 public:
  /**
   * \brief Ordered list of consecutive pages.
   */
  std::vector<PageId> pages;

  std::set<PageId> selectEveryOther(const PageId& base) const;
};


#endif
