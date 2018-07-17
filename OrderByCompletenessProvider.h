#ifndef SCANTAILOR_ADVANCED_ORDERBYCOMPLETENESS_H
#define SCANTAILOR_ADVANCED_ORDERBYCOMPLETENESS_H


#include "PageOrderProvider.h"

class OrderByCompletenessProvider : public PageOrderProvider {
 public:
  OrderByCompletenessProvider() = default;

  bool precedes(const PageId&, bool lhs_incomplete, const PageId&, bool rhs_incomplete) const override;
};


#endif  // SCANTAILOR_ADVANCED_ORDERBYCOMPLETENESS_H
