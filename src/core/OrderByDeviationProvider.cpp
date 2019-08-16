// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "OrderByDeviationProvider.h"

OrderByDeviationProvider::OrderByDeviationProvider(const DeviationProvider<PageId>& deviationProvider)
    : m_deviationProvider(&deviationProvider) {}

bool OrderByDeviationProvider::precedes(const PageId& lhs_page,
                                        bool lhs_incomplete,
                                        const PageId& rhs_page,
                                        bool rhs_incomplete) const {
  if (lhs_incomplete != rhs_incomplete) {
    // Invalid (unknown) sizes go to the back.
    return lhs_incomplete;
  }

  return (m_deviationProvider->getDeviationValue(lhs_page) > m_deviationProvider->getDeviationValue(rhs_page));
}
