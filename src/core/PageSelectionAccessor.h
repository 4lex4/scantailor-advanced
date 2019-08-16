// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

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
  intrusive_ptr<const PageSelectionProvider> m_provider;
};


#endif
