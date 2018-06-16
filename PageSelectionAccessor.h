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

#ifndef PAGE_SELECTION_ACCESSOR_H_
#define PAGE_SELECTION_ACCESSOR_H_

#include <set>
#include <vector>
#include "PageId.h"
#include "PageRange.h"
#include "PageSelectionProvider.h"
#include "intrusive_ptr.h"

class PageSequence;

class PageSelectionAccessor {
  // Member-wise copying is OK.
 public:
  explicit PageSelectionAccessor(intrusive_ptr<const PageSelectionProvider> provider);

  PageSequence allPages() const;

  std::set<PageId> selectedPages() const;

  std::vector<PageRange> selectedRanges() const;

 private:
  intrusive_ptr<const PageSelectionProvider> m_ptrProvider;
};


#endif
