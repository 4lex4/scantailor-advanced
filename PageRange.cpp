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

  const int base_idx = static_cast<const int>(it - pages.begin());
  int idx = 0;
  for (const PageId& page_id : pages) {
    if (((idx - base_idx) & 1) == 0) {
      selection.insert(page_id);
    }
    ++idx;
  }

  return selection;
}
