// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "SqDistApproximant.h"
#include "FrenetFrame.h"
#include "MatrixCalc.h"

namespace spfit {
SqDistApproximant::SqDistApproximant(const Vec2d& origin, const Vec2d& u, const Vec2d& v, double m, double n) {
  assert(std::fabs(u.squaredNorm() - 1.0) < 1e-06 && "u is not normalized");
  assert(std::fabs(v.squaredNorm() - 1.0) < 1e-06 && "v is not normalized");
  assert(std::fabs(u.dot(v)) < 1e-06 && "u and v are not orthogonal");

  // Consider the following equation:
  // w = R*x + t
  // w: vector in the local coordinate system.
  // R: rotation matrix.  Actually the inverse of [u v].
  // x: vector in the global coordinate system.
  // t: translation component.

  // R = | u1 u2 |
  // | v1 v2 |
  Mat22d R;
  R(0, 0) = u[0];
  R(0, 1) = u[1];
  R(1, 0) = v[0];
  R(1, 1) = v[1];

  StaticMatrixCalc<double, 4> mc;
  Vec2d t;  // Translation component.
  (-(mc(R) * mc(origin, 2, 1))).write(t);

  A(0, 0) = m * R(0, 0) * R(0, 0) + n * R(1, 0) * R(1, 0);
  A(0, 1) = A(1, 0) = m * R(0, 0) * R(0, 1) + n * R(1, 0) * R(1, 1);
  A(1, 1) = m * R(0, 1) * R(0, 1) + n * R(1, 1) * R(1, 1);
  b[0] = 2 * (m * t[0] * R(0, 0) + n * t[1] * R(1, 0));
  b[1] = 2 * (m * t[0] * R(0, 1) + n * t[1] * R(1, 1));
  c = m * t[0] * t[0] + n * t[1] * t[1];
}

SqDistApproximant SqDistApproximant::pointDistance(const Vec2d& pt) {
  return weightedPointDistance(pt, 1);
}

SqDistApproximant SqDistApproximant::weightedPointDistance(const Vec2d& pt, double weight) {
  return SqDistApproximant(pt, Vec2d(1, 0), Vec2d(0, 1), weight, weight);
}

SqDistApproximant SqDistApproximant::lineDistance(const QLineF& line) {
  return weightedLineDistance(line, 1);
}

SqDistApproximant SqDistApproximant::weightedLineDistance(const QLineF& line, double weight) {
  Vec2d u(line.p2() - line.p1());
  const double sqlen = u.squaredNorm();
  if (sqlen > 1e-6) {
    u /= std::sqrt(sqlen);
  } else {
    return pointDistance(line.p1());
  }

  // Unit normal to line.
  const Vec2d v(-u[1], u[0]);

  return SqDistApproximant(line.p1(), u, v, 0, weight);
}

SqDistApproximant SqDistApproximant::curveDistance(const Vec2d& referencePoint,
                                                   const FrenetFrame& frenetFrame,
                                                   double signedCurvature) {
  return weightedCurveDistance(referencePoint, frenetFrame, signedCurvature, 1);
}

SqDistApproximant SqDistApproximant::weightedCurveDistance(const Vec2d& referencePoint,
                                                           const FrenetFrame& frenetFrame,
                                                           const double signedCurvature,
                                                           const double weight) {
  const double absCurvature = std::fabs(signedCurvature);
  double m = 0;

  if (absCurvature > std::numeric_limits<double>::epsilon()) {
    const Vec2d toReferencePoint(referencePoint - frenetFrame.origin());
    const double p = 1.0 / absCurvature;
    const double d = std::fabs(frenetFrame.unitNormal().dot(toReferencePoint));
    m = d / (d + p);  // Formula 7 in [2].
  }

  return SqDistApproximant(frenetFrame.origin(), frenetFrame.unitTangent(), frenetFrame.unitNormal(), m * weight,
                           weight);
}

double SqDistApproximant::evaluate(const Vec2d& pt) const {
  StaticMatrixCalc<double, 8, 1> mc;

  return (mc(pt, 1, 2) * mc(A) * mc(pt, 2, 1) + mc(b, 1, 2) * mc(pt, 2, 1)).rawData()[0] + c;
}
}  // namespace spfit