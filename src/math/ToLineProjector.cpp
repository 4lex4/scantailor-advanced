// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "ToLineProjector.h"
#include <cmath>

ToLineProjector::ToLineProjector(const QLineF& line) : m_origin(line.p1()), m_vec(line.p2() - line.p1()), m_mat(m_vec) {
  using namespace std;
  // At*A*x = At*b
  const double AtA = m_mat.dot(m_mat);

  if (std::abs(AtA) > numeric_limits<double>::epsilon()) {
    // x = (At*A)-1 * At
    m_mat /= AtA;
  } else {
    m_mat[0] = 0;
    m_mat[1] = 0;
  }
}

double ToLineProjector::projectionScalar(const QPointF& pt) const {
  const Vec2d b(pt - m_origin);

  return m_mat.dot(b);
}

QPointF ToLineProjector::projectionPoint(const QPointF& pt) const {
  return m_origin + m_vec * projectionScalar(pt);
}

QPointF ToLineProjector::projectionVector(const QPointF& pt) const {
  return projectionPoint(pt) - pt;
}

double ToLineProjector::projectionDist(const QPointF& pt) const {
  return std::sqrt(projectionSqDist(pt));
}

double ToLineProjector::projectionSqDist(const QPointF& pt) const {
  return Vec2d(projectionPoint(pt) - pt).squaredNorm();
}
