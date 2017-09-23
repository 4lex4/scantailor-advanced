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
#include "Params.h"

namespace page_layout {
    OrderByWidthProvider::OrderByWidthProvider(IntrusivePtr<Settings> const& settings)
            : m_ptrSettings(settings) {
    }

    bool OrderByWidthProvider::precedes(PageId const& lhs_page,
                                        bool const lhs_incomplete,
                                        PageId const& rhs_page,
                                        bool const rhs_incomplete) const {
        std::unique_ptr<Params> const lhs_params(m_ptrSettings->getPageParams(lhs_page));
        std::unique_ptr<Params> const rhs_params(m_ptrSettings->getPageParams(rhs_page));

        QSizeF lhs_size;
        if (lhs_params.get()) {
            Margins const margins(lhs_params->hardMarginsMM());
            lhs_size = lhs_params->contentSizeMM();
            lhs_size += QSizeF(
                    margins.left() + margins.right(), margins.top() + margins.bottom()
            );
        }
        QSizeF rhs_size;
        if (rhs_params.get()) {
            Margins const margins(rhs_params->hardMarginsMM());
            rhs_size = rhs_params->contentSizeMM();
            rhs_size += QSizeF(
                    margins.left() + margins.right(), margins.top() + margins.bottom()
            );
        }

        bool const lhs_valid = !lhs_incomplete && lhs_size.isValid();
        bool const rhs_valid = !rhs_incomplete && rhs_size.isValid();

        if (lhs_valid != rhs_valid) {
            return lhs_valid;
        }

        return lhs_size.width() < rhs_size.width();
    }
}  // namespace page_layout