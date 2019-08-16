// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "OrderByWidthProvider.h"

#include <utility>
#include "Params.h"

namespace page_layout {
OrderByWidthProvider::OrderByWidthProvider(intrusive_ptr<Settings> settings) : m_settings(std::move(settings)) {}

bool OrderByWidthProvider::precedes(const PageId& lhs_page,
                                    const bool lhs_incomplete,
                                    const PageId& rhs_page,
                                    const bool rhs_incomplete) const {
  const std::unique_ptr<Params> lhs_params(m_settings->getPageParams(lhs_page));
  const std::unique_ptr<Params> rhs_params(m_settings->getPageParams(rhs_page));

  QSizeF lhs_size;
  if (lhs_params) {
    const Margins margins(lhs_params->hardMarginsMM());
    lhs_size = lhs_params->contentSizeMM();
    lhs_size += QSizeF(margins.left() + margins.right(), margins.top() + margins.bottom());
  }
  QSizeF rhs_size;
  if (rhs_params) {
    const Margins margins(rhs_params->hardMarginsMM());
    rhs_size = rhs_params->contentSizeMM();
    rhs_size += QSizeF(margins.left() + margins.right(), margins.top() + margins.bottom());
  }

  const bool lhs_valid = !lhs_incomplete && lhs_size.isValid();
  const bool rhs_valid = !rhs_incomplete && rhs_size.isValid();

  if (lhs_valid != rhs_valid) {
    // Invalid (unknown) sizes go to the back.
    return lhs_valid;
  }

  return lhs_size.width() < rhs_size.width();
}  // OrderByWidthProvider::precedes
}  // namespace page_layout