/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C)  Joseph Artsimovich <joseph.artsimovich@gmail.com>
    Copyright (C)  Vadim Kuznetsov ()DikBSD <dikbsd@gmail.com>

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

#ifndef PAGE_SPLIT_ORDER_BY_SPLIT_TYPE_PROVIDER_H_
#define PAGE_SPLIT_ORDER_BY_SPLIT_TYPE_PROVIDER_H_

#include "PageOrderProvider.h"
#include "Settings.h"
#include "intrusive_ptr.h"

namespace page_split {
class OrderBySplitTypeProvider : public PageOrderProvider {
 public:
  explicit OrderBySplitTypeProvider(intrusive_ptr<Settings> settings);

  bool precedes(const PageId& lhs_page,
                bool lhs_incomplete,
                const PageId& rhs_page,
                bool rhs_incomplete) const override;

 private:
  intrusive_ptr<Settings> m_ptrSettings;
};
}  // namespace page_split

#endif  // PAGE_SPLIT_ORDER_BY_SPLIT_TYPE_PROVIDER_H_
