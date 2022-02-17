// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "PolylineIntersector.h"

#include <cmath>

#include "ToLineProjector.h"

PolylineIntersector::Hint::Hint() : m_lastSegment(0), m_direction(1) {}

void PolylineIntersector::Hint::update(int newSegment) {
  m_direction = newSegment < m_lastSegment ? -1 : 1;
  m_lastSegment = newSegment;
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
  int leftIdx = 0;
  auto rightIdx = static_cast<int>(m_polyline.size() - 1);
  double leftDot = nv.dot(m_polyline[leftIdx] - origin);

  while (leftIdx + 1 < rightIdx) {
    const int midIdx = (leftIdx + rightIdx) >> 1;
    const double midDot = nv.dot(m_polyline[midIdx] - origin);

    if (midDot * leftDot <= 0) {
      // Note: <= 0 vs < 0 is actually important for this branch.
      // 0 would indicate either left or mid point is our exact answer.
      rightIdx = midIdx;
    } else {
      leftIdx = midIdx;
      leftDot = midDot;
    }
  }

  hint.update(leftIdx);
  return intersectWithSegment(line, leftIdx);
}  // PolylineIntersector::intersect

bool PolylineIntersector::intersectsSegment(const QLineF& normal, int segment) const {
  if ((segment < 0) || (segment >= m_numSegments)) {
    return false;
  }

  const QLineF segLine(m_polyline[segment], m_polyline[segment + 1]);
  return intersectsSpan(normal, segLine);
}

bool PolylineIntersector::intersectsSpan(const QLineF& normal, const QLineF& span) const {
  const Vec2d v1(normal.p2() - normal.p1());
  const Vec2d v2(span.p1() - normal.p1());
  const Vec2d v3(span.p2() - normal.p1());
  return v1.dot(v2) * v1.dot(v3) <= 0;
}

QPointF PolylineIntersector::intersectWithSegment(const QLineF& line, int segment) const {
  const QLineF segLine(m_polyline[segment], m_polyline[segment + 1]);
  QPointF intersection;
#if QT_VERSION_MAJOR  == 5
  auto intersect = line.intersect(segLine, &intersection);
#else
  auto intersect = line.intersects(segLine, &intersection);
#endif
  if (intersect == QLineF::NoIntersection) {
    // Considering we were called for a reason, the segment must
    // be on the same line as our subject line.  Just return segment
    // midpoint in this case.
    return segLine.pointAt(0.5);
  }
  return intersection;
}

bool PolylineIntersector::tryIntersectingOutsideOfPolyline(const QLineF& line,
                                                           QPointF& intersection,
                                                           Hint& hint) const {
  const QLineF normal(line.normalVector());
  const QPointF origin(normal.p1());
  const Vec2d nv(normal.p2() - normal.p1());

  const Vec2d frontVec(m_polyline.front() - origin);
  const Vec2d backVec(m_polyline.back() - origin);
  const double frontDot = nv.dot(frontVec);
  const double backDot = nv.dot(backVec);

  if (frontDot * backDot <= 0) {
    return false;
  }

  const ToLineProjector proj(line);

  if (std::fabs(frontDot) < std::fabs(backDot)) {
    hint.update(-1);
    intersection = proj.projectionPoint(m_polyline.front());
  } else {
    hint.update(m_numSegments);
    intersection = proj.projectionPoint(m_polyline.back());
  }
  return true;
}
