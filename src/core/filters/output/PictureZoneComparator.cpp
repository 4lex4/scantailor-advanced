// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "PictureZoneComparator.h"

#include "PictureLayerProperty.h"
#include "ZoneSet.h"

namespace output {
bool PictureZoneComparator::equal(const ZoneSet& lhs, const ZoneSet& rhs) {
  ZoneSet::const_iterator lhsIt(lhs.begin());
  ZoneSet::const_iterator rhsIt(rhs.begin());
  const ZoneSet::const_iterator lhsEnd(lhs.end());
  const ZoneSet::const_iterator rhsEnd(rhs.end());
  for (; lhsIt != lhsEnd && rhsIt != rhsEnd; ++lhsIt, ++rhsIt) {
    if (!equal(*lhsIt, *rhsIt)) {
      return false;
    }
  }
  return lhsIt == lhsEnd && rhsIt == rhsEnd;
}

bool PictureZoneComparator::equal(const Zone& lhs, const Zone& rhs) {
  if (lhs.spline().toPolygon() != rhs.spline().toPolygon()) {
    return false;
  }
  return equal(lhs.properties(), rhs.properties());
}

bool PictureZoneComparator::equal(const PropertySet& lhs, const PropertySet& rhs) {
  using PLP = PictureLayerProperty;
  return lhs.locateOrDefault<PLP>()->layer() == rhs.locateOrDefault<PLP>()->layer();
}
}  // namespace output