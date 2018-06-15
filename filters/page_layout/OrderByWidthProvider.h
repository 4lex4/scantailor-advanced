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

#ifndef PAGE_LAYOUT_ORDER_BY_WIDTH_PROVIDER_H_
#define PAGE_LAYOUT_ORDER_BY_WIDTH_PROVIDER_H_

#include "PageOrderProvider.h"
#include "Settings.h"
#include "intrusive_ptr.h"

namespace page_layout {
class OrderByWidthProvider : public PageOrderProvider {
 public:
  explicit OrderByWidthProvider(intrusive_ptr<Settings> settings);

  bool precedes(const PageId& lhs_page,
                bool lhs_incomplete,
                const PageId& rhs_page,
                bool rhs_incomplete) const override;

 private:
  intrusive_ptr<Settings> m_ptrSettings;
};
}  // namespace page_layout
#endif
