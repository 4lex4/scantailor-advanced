// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_FOUNDATION_LINEINTERSECTIONSCALAR_H_
#define SCANTAILOR_FOUNDATION_LINEINTERSECTIONSCALAR_H_

#include <QLineF>

/**
 * Finds such scalars s1 and s2, so that "line1.pointAt(s1)" and "line2.pointAt(s2)"
 * would be the intersection point between line1 and line2.  Returns false if the
 * lines are parallel or if any of the lines have zero length and therefore no direction.
 */
bool lineIntersectionScalar(const QLineF& line1, const QLineF& line2, double& s1, double& s2);

/**
 * Same as the one above, but doesn't bother to calculate s2.
 */
bool lineIntersectionScalar(const QLineF& line1, const QLineF& line2, double& s1);

#endif
