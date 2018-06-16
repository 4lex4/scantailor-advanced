/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2007-2009  Joseph Artsimovich <joseph_a@mail.ru>

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

#include "Proximity.h"
#include <QLineF>
#include <QPointF>

Proximity::Proximity(const QPointF& p1, const QPointF& p2) {
  const double dx = p1.x() - p2.x();
  const double dy = p1.y() - p2.y();
  m_sqDist = dx * dx + dy * dy;  // dx * dy;
}

Proximity Proximity::pointAndLineSegment(const QPointF& pt, const QLineF& segment, QPointF* point_on_segment) {
  if (segment.p1() == segment.p2()) {
    // Line segment is zero length.
    if (point_on_segment) {
      *point_on_segment = segment.p1();
    }

    return Proximity(pt, segment.p1());
  }

  QLineF perpendicular(segment.normalVector());

  // Make the perpendicular pass through pt.
  perpendicular.translate(-perpendicular.p1());
  perpendicular.translate(pt);
  // Calculate intersection.
  QPointF intersection;
  segment.intersect(perpendicular, &intersection);

  const double dx1 = segment.p1().x() - intersection.x();
  const double dy1 = segment.p1().y() - intersection.y();
  const double dx2 = segment.p2().x() - intersection.x();
  const double dy2 = segment.p2().y() - intersection.y();
  const double dx12 = dx1 * dx2;
  const double dy12 = dy1 * dy2;
  if ((dx12 < 0.0) || (dy12 < 0.0) || ((dx12 == 0.0) && (dy12 == 0.0))) {
    // Intersection is on the segment.
    if (point_on_segment) {
      *point_on_segment = intersection;
    }

    return Proximity(intersection, pt);
  }

  Proximity prx[2];
  QPointF pts[2];

  prx[0] = Proximity(segment.p1(), pt);
  prx[1] = Proximity(segment.p2(), pt);
  pts[0] = segment.p1();
  pts[1] = segment.p2();

  const Proximity* p_min_prx = std::min_element(prx, prx + 2);
  if (point_on_segment) {
    *point_on_segment = pts[p_min_prx - prx];
  }

  return *p_min_prx;
}  // Proximity::pointAndLineSegment
