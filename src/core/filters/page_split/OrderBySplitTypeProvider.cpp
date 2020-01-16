// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "OrderBySplitTypeProvider.h"

#include <cassert>
#include <utility>

namespace page_split {
OrderBySplitTypeProvider::OrderBySplitTypeProvider(intrusive_ptr<Settings> settings)
    : m_settings(std::move(settings)) {}

bool OrderBySplitTypeProvider::precedes(const PageId& lhsPage,
                                        const bool lhsIncomplete,
                                        const PageId& rhsPage,
                                        const bool rhsIncomplete) const {
  if (lhsIncomplete != rhsIncomplete) {
    // Pages with question mark go to the bottom.
    return rhsIncomplete;
  } else if (lhsIncomplete) {
    assert(rhsIncomplete);
    // Two pages with question marks are ordered naturally.
    return lhsPage < rhsPage;
  }

  assert(!lhsIncomplete);
  assert(!rhsIncomplete);

  const Settings::Record lhsRecord(m_settings->getPageRecord(lhsPage.imageId()));
  const Settings::Record rhsRecord(m_settings->getPageRecord(rhsPage.imageId()));

  const Params* lhsParams = lhsRecord.params();
  const Params* rhsParams = rhsRecord.params();

  int lhsLayoutType = lhsRecord.combinedLayoutType();
  if (lhsParams) {
    lhsLayoutType = lhsParams->pageLayout().toLayoutType();
  }
  if (lhsLayoutType == AUTO_LAYOUT_TYPE) {
    lhsLayoutType = 100;  // To force it below pages with known layout.
  }

  int rhsLayoutType = rhsRecord.combinedLayoutType();
  if (rhsParams) {
    rhsLayoutType = rhsParams->pageLayout().toLayoutType();
  }
  if (rhsLayoutType == AUTO_LAYOUT_TYPE) {
    rhsLayoutType = 100;  // To force it below pages with known layout.
  }

  if (lhsLayoutType == rhsLayoutType) {
    return lhsPage < rhsPage;
  } else {
    return lhsLayoutType < rhsLayoutType;
  }
}  // OrderBySplitTypeProvider::precedes
}  // namespace page_split