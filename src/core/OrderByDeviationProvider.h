// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_ORDERBYDEVIATIONPROVIDER_H
#define SCANTAILOR_ORDERBYDEVIATIONPROVIDER_H

#include "DeviationProvider.h"
#include "PageId.h"
#include "PageOrderProvider.h"

class OrderByDeviationProvider : public PageOrderProvider {
 public:
  explicit OrderByDeviationProvider(const DeviationProvider<PageId>& deviationProvider);

  bool precedes(const PageId& lhs_page,
                bool lhs_incomplete,
                const PageId& rhs_page,
                bool rhs_incomplete) const override;

 private:
  const DeviationProvider<PageId>* m_deviationProvider;
};


#endif  // SCANTAILOR_ORDERBYDEVIATIONPROVIDER_H
