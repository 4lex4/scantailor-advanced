// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "LineIntersectionScalar.h"
#include <cmath>

bool lineIntersectionScalar(const QLineF& line1, const QLineF& line2, double& s1, double& s2) {
  const QPointF p1(line1.p1());
  const QPointF p2(line2.p1());
  const QPointF v1(line1.p2() - line1.p1());
  const QPointF v2(line2.p2() - line2.p1());
  // p1 + s1 * v1 = p2 + s2 * v2
  // which gives us a system of equations:
  // s1 * v1.x - s2 * v2.x = p2.x - p1.x
  // s1 * v1.y - s2 * v2.y = p2.y - p1.y
  // In matrix form:
  // [v1 -v2]*x = p2 - p1
  // Taking A = [v1 -v2], b = p2 - p1, and solving it by Cramer's rule:
  // s1 = |b -v2|/|A|
  // s2 = |v1  b|/|A|
  const double det_A = v2.x() * v1.y() - v1.x() * v2.y();
  if (std::fabs(det_A) < std::numeric_limits<double>::epsilon()) {
    return false;
  }

  const double r_det_A = 1.0 / det_A;
  const QPointF b(p2 - p1);
  s1 = (v2.x() * b.y() - b.x() * v2.y()) * r_det_A;
  s2 = (v1.x() * b.y() - b.x() * v1.y()) * r_det_A;

  return true;
}

bool lineIntersectionScalar(const QLineF& line1, const QLineF& line2, double& s1) {
  const QPointF p1(line1.p1());
  const QPointF p2(line2.p1());
  const QPointF v1(line1.p2() - line1.p1());
  const QPointF v2(line2.p2() - line2.p1());

  // p1 + s1 * v1 = p2 + s2 * v2
  // which gives us a system of equations:
  // s1 * v1.x - s2 * v2.x = p2.x - p1.x
  // s1 * v1.y - s2 * v2.y = p2.y - p1.y
  // In matrix form:
  // [v1 -v2]*x = p2 - p1
  // Taking A = [v1 -v2], b = p2 - p1, and solving it by Cramer's rule:
  // s1 = |b -v2|/|A|
  // s2 = |v1  b|/|A|
  const double det_A = v2.x() * v1.y() - v1.x() * v2.y();
  if (std::fabs(det_A) < std::numeric_limits<double>::epsilon()) {
    return false;
  }

  const QPointF b(p2 - p1);
  s1 = (v2.x() * b.y() - b.x() * v2.y()) / det_A;

  return true;
}
