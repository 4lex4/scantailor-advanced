// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef PAGE_ORDER_PROVIDER_H_
#define PAGE_ORDER_PROVIDER_H_

#include "ref_countable.h"

class PageId;

/**
 * A base class for different page ordering strategies.
 */
class PageOrderProvider : public ref_countable {
 public:
  /**
   * Returns true if \p lhs_page precedes \p rhs_page.
   * \p lhs_incomplete and \p rhs_incomplete indicate whether
   * a page is represented by IncompleteThumbnail.
   */
  virtual bool precedes(const PageId& lhs_page,
                        bool lhs_incomplete,
                        const PageId& rhs_page,
                        bool rhs_incomplete) const = 0;
};


#endif
