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

#include "PolylineIntersector.h"
#include <cmath>
#include "ToLineProjector.h"

PolylineIntersector::Hint::Hint() : m_lastSegment(0), m_direction(1) {}

void PolylineIntersector::Hint::update(int new_segment) {
  m_direction = new_segment < m_lastSegment ? -1 : 1;
  m_lastSegment = new_segment;
}

PolylineIntersector::PolylineIntersector(const std::vector<QPointF>& polyline)
    : m_polyline(polyline), m_numSegments(static_cast<int>(polyline.size() - 1)) {}

QPointF PolylineIntersector::intersect(const QLineF& line, Hint& hint) const {
  const QLineF normal(line.normalVector());

  if (intersectsSegment(normal, hint.m_lastSegment)) {
    return intersectWithSegment(line, hint.m_lastSegment);
  }

  int segment;

  // Check the next segment in direction provided by hint.
  if (intersectsSegment(normal, (segment = hint.m_lastSegment + hint.m_direction))) {
    hint.update(segment);

    return intersectWithSegment(line, segment);
  }

  // Check the next segment in opposite direction.
  if (intersectsSegment(normal, (segment = hint.m_lastSegment - hint.m_direction))) {
    hint.update(segment);

    return intersectWithSegment(line, segment);
  }

  // Does the whole polyline intersect our line?
  QPointF intersection;
  if (tryIntersectingOutsideOfPolyline(line, intersection, hint)) {
    return intersection;
  }
  // OK, let's do a binary search then.
  const QPointF origin(normal.p1());
  const Vec2d nv(normal.p2() - normal.p1());
  int left_idx = 0;
  auto right_idx = static_cast<int>(m_polyline.size() - 1);
  double left_dot = nv.dot(m_polyline[left_idx] - origin);

  while (left_idx + 1 < right_idx) {
    const int mid_idx = (left_idx + right_idx) >> 1;
    const double mid_dot = nv.dot(m_polyline[mid_idx] - origin);

    if (mid_dot * left_dot <= 0) {
      // Note: <= 0 vs < 0 is actually important for this branch.
      // 0 would indicate either left or mid point is our exact answer.
      right_idx = mid_idx;
    } else {
      left_idx = mid_idx;
      left_dot = mid_dot;
    }
  }

  hint.update(left_idx);

  return intersectWithSegment(line, left_idx);
}  // PolylineIntersector::intersect

bool PolylineIntersector::intersectsSegment(const QLineF& normal, int segment) const {
  if ((segment < 0) || (segment >= m_numSegments)) {
    return false;
  }

  const QLineF seg_line(m_polyline[segment], m_polyline[segment + 1]);

  return intersectsSpan(normal, seg_line);
}

bool PolylineIntersector::intersectsSpan(const QLineF& normal, const QLineF& span) const {
  const Vec2d v1(normal.p2() - normal.p1());
  const Vec2d v2(span.p1() - normal.p1());
  const Vec2d v3(span.p2() - normal.p1());

  return v1.dot(v2) * v1.dot(v3) <= 0;
}

QPointF PolylineIntersector::intersectWithSegment(const QLineF& line, int segment) const {
  const QLineF seg_line(m_polyline[segment], m_polyline[segment + 1]);
  QPointF intersection;
  if (line.intersect(seg_line, &intersection) == QLineF::NoIntersection) {
    // Considering we were called for a reason, the segment must
    // be on the same line as our subject line.  Just return segment
    // midpoint in this case.
    return seg_line.pointAt(0.5);
  }

  return intersection;
}

bool PolylineIntersector::tryIntersectingOutsideOfPolyline(const QLineF& line,
                                                           QPointF& intersection,
                                                           Hint& hint) const {
  const QLineF normal(line.normalVector());
  const QPointF origin(normal.p1());
  const Vec2d nv(normal.p2() - normal.p1());

  const Vec2d front_vec(m_polyline.front() - origin);
  const Vec2d back_vec(m_polyline.back() - origin);
  const double front_dot = nv.dot(front_vec);
  const double back_dot = nv.dot(back_vec);

  if (front_dot * back_dot <= 0) {
    return false;
  }

  const ToLineProjector proj(line);

  if (std::fabs(front_dot) < std::fabs(back_dot)) {
    hint.update(-1);
    intersection = proj.projectionPoint(m_polyline.front());
  } else {
    hint.update(m_numSegments);
    intersection = proj.projectionPoint(m_polyline.back());
  }

  return true;
}
