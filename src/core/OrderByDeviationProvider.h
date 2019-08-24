// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_CORE_ORDERBYDEVIATIONPROVIDER_H_
#define SCANTAILOR_CORE_ORDERBYDEVIATIONPROVIDER_H_

#include "DeviationProvider.h"
#include "PageId.h"
#include "PageOrderProvider.h"

class OrderByDeviationProvider : public PageOrderProvider {
 public:
  explicit OrderByDeviationProvider(const DeviationProvider<PageId>& deviationProvider);

  bool precedes(const PageId& lhsPage, bool lhsIncomplete, const PageId& rhsPage, bool rhsIncomplete) const override;

 private:
  const DeviationProvider<PageId>* m_deviationProvider;
};


#endif  // SCANTAILOR_CORE_ORDERBYDEVIATIONPROVIDER_H_
