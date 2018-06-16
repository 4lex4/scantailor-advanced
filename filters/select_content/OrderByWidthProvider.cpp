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

#include "OrderByWidthProvider.h"

#include <utility>

namespace select_content {
OrderByWidthProvider::OrderByWidthProvider(intrusive_ptr<Settings> settings) : m_ptrSettings(std::move(settings)) {}

bool OrderByWidthProvider::precedes(const PageId& lhs_page,
                                    const bool lhs_incomplete,
                                    const PageId& rhs_page,
                                    const bool rhs_incomplete) const {
  const std::unique_ptr<Params> lhs_params(m_ptrSettings->getPageParams(lhs_page));
  const std::unique_ptr<Params> rhs_params(m_ptrSettings->getPageParams(rhs_page));

  QSizeF lhs_size;
  if (lhs_params) {
    lhs_size = lhs_params->contentSizeMM();
  }
  QSizeF rhs_size;
  if (rhs_params) {
    rhs_size = rhs_params->contentSizeMM();
  }

  const bool lhs_valid = !lhs_incomplete && lhs_size.isValid();
  const bool rhs_valid = !rhs_incomplete && rhs_size.isValid();

  if (lhs_valid != rhs_valid) {
    // Invalid (unknown) sizes go to the back.
    return lhs_valid;
  }

  return lhs_size.width() < rhs_size.width();
}
}  // namespace select_content