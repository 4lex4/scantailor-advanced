// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "PageRange.h"

std::set<PageId> PageRange::selectEveryOther(const PageId& base) const {
  std::set<PageId> selection;

  auto it(pages.begin());
  const auto end(pages.end());
  for (; it != end && *it != base; ++it) {
    // Continue until we have a match.
  }
  if (it == end) {
    return selection;
  }

  const int base_idx = static_cast<int>(it - pages.begin());
  int idx = 0;
  for (const PageId& page_id : pages) {
    if (((idx - base_idx) & 1) == 0) {
      selection.insert(page_id);
    }
    ++idx;
  }

  return selection;
}
