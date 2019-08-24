// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_CORE_ORDERBYCOMPLETENESSPROVIDER_H_
#define SCANTAILOR_CORE_ORDERBYCOMPLETENESSPROVIDER_H_

#include "PageOrderProvider.h"

class OrderByCompletenessProvider : public PageOrderProvider {
 public:
  OrderByCompletenessProvider() = default;

  bool precedes(const PageId&, bool lhsIncomplete, const PageId&, bool rhsIncomplete) const override;
};


#endif  // SCANTAILOR_CORE_ORDERBYCOMPLETENESSPROVIDER_H_
