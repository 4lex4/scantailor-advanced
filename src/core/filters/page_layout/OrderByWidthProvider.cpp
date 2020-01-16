// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "OrderByWidthProvider.h"

#include <utility>

#include "Params.h"

namespace page_layout {
OrderByWidthProvider::OrderByWidthProvider(intrusive_ptr<Settings> settings) : m_settings(std::move(settings)) {}

bool OrderByWidthProvider::precedes(const PageId& lhsPage,
                                    const bool lhsIncomplete,
                                    const PageId& rhsPage,
                                    const bool rhsIncomplete) const {
  const std::unique_ptr<Params> lhsParams(m_settings->getPageParams(lhsPage));
  const std::unique_ptr<Params> rhsParams(m_settings->getPageParams(rhsPage));

  QSizeF lhsSize;
  if (lhsParams) {
    const Margins margins(lhsParams->hardMarginsMM());
    lhsSize = lhsParams->contentSizeMM();
    lhsSize += QSizeF(margins.left() + margins.right(), margins.top() + margins.bottom());
  }
  QSizeF rhsSize;
  if (rhsParams) {
    const Margins margins(rhsParams->hardMarginsMM());
    rhsSize = rhsParams->contentSizeMM();
    rhsSize += QSizeF(margins.left() + margins.right(), margins.top() + margins.bottom());
  }

  const bool lhsValid = !lhsIncomplete && lhsSize.isValid();
  const bool rhsValid = !rhsIncomplete && rhsSize.isValid();

  if (lhsValid != rhsValid) {
    // Invalid (unknown) sizes go to the back.
    return lhsValid;
  }
  return lhsSize.width() < rhsSize.width();
}  // OrderByWidthProvider::precedes
}  // namespace page_layout