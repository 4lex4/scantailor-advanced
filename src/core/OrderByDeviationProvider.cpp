
#include "OrderByDeviationProvider.h"

OrderByDeviationProvider::OrderByDeviationProvider(const DeviationProvider<PageId>& deviationProvider)
    : m_deviationProvider(&deviationProvider) {}

bool OrderByDeviationProvider::precedes(const PageId& lhs_page,
                                        bool lhs_incomplete,
                                        const PageId& rhs_page,
                                        bool rhs_incomplete) const {
  if (lhs_incomplete != rhs_incomplete) {
    // Invalid (unknown) sizes go to the back.
    return lhs_incomplete;
  }

  return (m_deviationProvider->getDeviationValue(lhs_page) > m_deviationProvider->getDeviationValue(rhs_page));
}
