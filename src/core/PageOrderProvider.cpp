// Copyright (C) 2020  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "PageOrderProvider.h"

intrusive_ptr<const PageOrderProvider> PageOrderProvider::reversed() const {
  class ReversedPageOrderProvider : public PageOrderProvider {
   public:
    explicit ReversedPageOrderProvider(const PageOrderProvider* parent) : m_parent(parent) {}

    bool precedes(const PageId& lhsPage, bool lhsIncomplete, const PageId& rhsPage, bool rhsIncomplete) const override {
      return m_parent->precedes(rhsPage, rhsIncomplete, lhsPage, lhsIncomplete);
    }

   private:
    const intrusive_ptr<const PageOrderProvider> m_parent;
  };
  return make_intrusive<ReversedPageOrderProvider>(this);
}
