/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C)  Joseph Artsimovich <joseph.artsimovich@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "FillZoneComparator.h"
#include "FillColorProperty.h"
#include "ZoneSet.h"
#include "imageproc/PolygonUtils.h"

using namespace imageproc;

namespace output {
bool FillZoneComparator::equal(const ZoneSet& lhs, const ZoneSet& rhs) {
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

bool FillZoneComparator::equal(const Zone& lhs, const Zone& rhs) {
  if (!PolygonUtils::fuzzyCompare(lhs.spline().toPolygon(), rhs.spline().toPolygon())) {
    return false;
  }

  return equal(lhs.properties(), rhs.properties());
}

bool FillZoneComparator::equal(const PropertySet& lhs, const PropertySet& rhs) {
  typedef FillColorProperty FCP;

  return lhs.locateOrDefault<FCP>()->color() == rhs.locateOrDefault<FCP>()->color();
}
}  // namespace output