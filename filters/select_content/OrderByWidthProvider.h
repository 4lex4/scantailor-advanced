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

#ifndef SELECT_CONTENT_ORDER_BY_WIDTH_PROVIDER_H_
#define SELECT_CONTENT_ORDER_BY_WIDTH_PROVIDER_H_

#include "Settings.h"
#include "IntrusivePtr.h"
#include "PageOrderProvider.h"

namespace select_content {
    class OrderByWidthProvider : public PageOrderProvider {
    public:
        OrderByWidthProvider(IntrusivePtr<Settings> const& settings);

        virtual bool precedes(PageId const& lhs_page, bool lhs_incomplete, PageId const& rhs_page,
                              bool rhs_incomplete) const;

    private:
        IntrusivePtr<Settings> m_ptrSettings;
    };
}
#endif
