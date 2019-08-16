// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

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
  intrusive_ptr<Settings> m_settings;
};
}  // namespace page_layout
#endif
