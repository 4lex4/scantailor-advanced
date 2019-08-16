// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "OrderBySplitTypeProvider.h"
#include <cassert>
#include <utility>

namespace page_split {
OrderBySplitTypeProvider::OrderBySplitTypeProvider(intrusive_ptr<Settings> settings)
    : m_settings(std::move(settings)) {}

bool OrderBySplitTypeProvider::precedes(const PageId& lhs_page,
                                        const bool lhs_incomplete,
                                        const PageId& rhs_page,
                                        const bool rhs_incomplete) const {
  if (lhs_incomplete != rhs_incomplete) {
    // Pages with question mark go to the bottom.
    return rhs_incomplete;
  } else if (lhs_incomplete) {
    assert(rhs_incomplete);
    // Two pages with question marks are ordered naturally.
    return lhs_page < rhs_page;
  }

  assert(!lhs_incomplete);
  assert(!rhs_incomplete);

  const Settings::Record lhs_record(m_settings->getPageRecord(lhs_page.imageId()));
  const Settings::Record rhs_record(m_settings->getPageRecord(rhs_page.imageId()));

  const Params* lhs_params = lhs_record.params();
  const Params* rhs_params = rhs_record.params();

  int lhs_layout_type = lhs_record.combinedLayoutType();
  if (lhs_params) {
    lhs_layout_type = lhs_params->pageLayout().toLayoutType();
  }
  if (lhs_layout_type == AUTO_LAYOUT_TYPE) {
    lhs_layout_type = 100;  // To force it below pages with known layout.
  }

  int rhs_layout_type = rhs_record.combinedLayoutType();
  if (rhs_params) {
    rhs_layout_type = rhs_params->pageLayout().toLayoutType();
  }
  if (rhs_layout_type == AUTO_LAYOUT_TYPE) {
    rhs_layout_type = 100;  // To force it below pages with known layout.
  }

  if (lhs_layout_type == rhs_layout_type) {
    return lhs_page < rhs_page;
  } else {
    return lhs_layout_type < rhs_layout_type;
  }
}  // OrderBySplitTypeProvider::precedes
}  // namespace page_split