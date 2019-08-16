// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_ADVANCED_ORDERBYCOMPLETENESS_H
#define SCANTAILOR_ADVANCED_ORDERBYCOMPLETENESS_H

#include "PageOrderProvider.h"

class OrderByCompletenessProvider : public PageOrderProvider {
 public:
  OrderByCompletenessProvider() = default;

  bool precedes(const PageId&, bool lhs_incomplete, const PageId&, bool rhs_incomplete) const override;
};


#endif  // SCANTAILOR_ADVANCED_ORDERBYCOMPLETENESS_H
