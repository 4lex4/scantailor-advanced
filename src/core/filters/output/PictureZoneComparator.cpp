// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "PictureZoneComparator.h"
#include "ZoneSet.h"
#include "PictureLayerProperty.h"

namespace output {
bool PictureZoneComparator::equal(const ZoneSet& lhs, const ZoneSet& rhs) {
  ZoneSet::const_iterator lhs_it(lhs.begin());
  ZoneSet::const_iterator rhs_it(rhs.begin());
  const ZoneSet::const_iterator lhs_end(lhs.end());
  const ZoneSet::const_iterator rhs_end(rhs.end());
  for (; lhs_it != lhs_end && rhs_it != rhs_end; ++lhs_it, ++rhs_it) {
    if (!equal(*lhs_it, *rhs_it)) {
      return false;
    }
  }

  return lhs_it == lhs_end && rhs_it == rhs_end;
}

bool PictureZoneComparator::equal(const Zone& lhs, const Zone& rhs) {
  if (lhs.spline().toPolygon() != rhs.spline().toPolygon()) {
    return false;
  }

  return equal(lhs.properties(), rhs.properties());
}

bool PictureZoneComparator::equal(const PropertySet& lhs, const PropertySet& rhs) {
  typedef PictureLayerProperty PLP;

  return lhs.locateOrDefault<PLP>()->layer() == rhs.locateOrDefault<PLP>()->layer();
}
}  // namespace output