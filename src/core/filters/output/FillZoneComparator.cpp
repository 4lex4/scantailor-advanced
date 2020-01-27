// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "FillZoneComparator.h"

#include <PolygonUtils.h>

#include "FillColorProperty.h"
#include "ZoneSet.h"

using namespace imageproc;

namespace output {
bool FillZoneComparator::equal(const ZoneSet& lhs, const ZoneSet& rhs) {
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

bool FillZoneComparator::equal(const Zone& lhs, const Zone& rhs) {
  if (!PolygonUtils::fuzzyCompare(lhs.spline().toPolygon(), rhs.spline().toPolygon())) {
    return false;
  }
  return equal(lhs.properties(), rhs.properties());
}

bool FillZoneComparator::equal(const PropertySet& lhs, const PropertySet& rhs) {
  using FCP = FillColorProperty;
  return lhs.locateOrDefault<FCP>()->color() == rhs.locateOrDefault<FCP>()->color();
}
}  // namespace output