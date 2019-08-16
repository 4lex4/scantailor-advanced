// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "OrderByCompletenessProvider.h"

bool OrderByCompletenessProvider::precedes(const PageId&, bool lhs_incomplete, const PageId&, bool rhs_incomplete) const {
  if (lhs_incomplete != rhs_incomplete) {
    // Incomplete pages go to the back.
    return rhs_incomplete;
  }
  return true;
}
