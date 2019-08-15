#include "OrderByCompletenessProvider.h"

bool OrderByCompletenessProvider::precedes(const PageId&, bool lhs_incomplete, const PageId&, bool rhs_incomplete) const {
  if (lhs_incomplete != rhs_incomplete) {
    // Incomplete pages go to the back.
    return rhs_incomplete;
  }
  return true;
}
