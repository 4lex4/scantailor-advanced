// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "Proximity.h"

#include <QLineF>
#include <QPointF>

#include <algorithm>

Proximity::Proximity(const QPointF& p1, const QPointF& p2) {
  const double dx = p1.x() - p2.x();
  const double dy = p1.y() - p2.y();
  m_sqDist = dx * dx + dy * dy;  // dx * dy;
}

Proximity Proximity::pointAndLineSegment(const QPointF& pt, const QLineF& segment, QPointF* pointOnSegment) {
  if (segment.p1() == segment.p2()) {
    // Line segment is zero length.
    if (pointOnSegment) {
      *pointOnSegment = segment.p1();
    }
    return Proximity(pt, segment.p1());
  }

  QLineF perpendicular(segment.normalVector());

  // Make the perpendicular pass through pt.
  perpendicular.translate(-perpendicular.p1());
  perpendicular.translate(pt);
  // Calculate intersection.
  QPointF intersection;
#if QT_VERSION_MAJOR  == 5 and QT_VERSION_MINOR < 14
  segment.intersect(perpendicular, &intersection);
#else
  segment.intersects(perpendicular, &intersection);
#endif

  const double dx1 = segment.p1().x() - intersection.x();
  const double dy1 = segment.p1().y() - intersection.y();
  const double dx2 = segment.p2().x() - intersection.x();
  const double dy2 = segment.p2().y() - intersection.y();
  const double dx12 = dx1 * dx2;
  const double dy12 = dy1 * dy2;
  if ((dx12 < 0.0) || (dy12 < 0.0) || ((dx12 == 0.0) && (dy12 == 0.0))) {
    // Intersection is on the segment.
    if (pointOnSegment) {
      *pointOnSegment = intersection;
    }
    return Proximity(intersection, pt);
  }

  Proximity prx[2];
  QPointF pts[2];

  prx[0] = Proximity(segment.p1(), pt);
  prx[1] = Proximity(segment.p2(), pt);
  pts[0] = segment.p1();
  pts[1] = segment.p2();

  const Proximity* minPrx = std::min_element(prx, prx + 2);
  if (pointOnSegment) {
    *pointOnSegment = pts[minPrx - prx];
  }
  return *minPrx;
}  // Proximity::pointAndLineSegment
