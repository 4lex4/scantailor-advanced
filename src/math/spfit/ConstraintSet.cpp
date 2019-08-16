// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "ConstraintSet.h"
#include <boost/foreach.hpp>
#include "FittableSpline.h"

namespace spfit {
ConstraintSet::ConstraintSet(const FittableSpline* spline) : m_spline(spline) {
  assert(spline);
}

void ConstraintSet::constrainControlPoint(int cp_idx, const QPointF& pos) {
  assert(cp_idx >= 0 && cp_idx < m_spline->numControlPoints());
  const QPointF cp(m_spline->controlPointPosition(cp_idx));

  // Fix x coordinate.
  LinearFunction f(m_spline->numControlPoints() * 2);
  f.a[cp_idx * 2] = 1;
  f.b = cp.x() - pos.x();
  m_constraints.push_back(f);
  // Fix y coordinate.
  f.a[cp_idx * 2] = 0;
  f.a[cp_idx * 2 + 1] = 1;
  f.b = cp.y() - pos.y();
  m_constraints.push_back(f);
}

void ConstraintSet::constrainSplinePoint(double t, const QPointF& pos) {
  std::vector<FittableSpline::LinearCoefficient> coeffs;
  m_spline->linearCombinationAt(t, coeffs);

  // Fix the x coordinate.
  LinearFunction f(m_spline->numControlPoints() * 2);
  f.b = -pos.x();
  for (const FittableSpline::LinearCoefficient& coeff : coeffs) {
    const int cp_idx = coeff.controlPointIdx;
    f.a[cp_idx * 2] = coeff.coeff;

    // Because we want a function from control point displacements, not positions.
    f.b += m_spline->controlPointPosition(cp_idx).x() * coeff.coeff;
  }
  m_constraints.push_back(f);

  // Fix the y coordinate.
  f.a.fill(0);
  f.b = -pos.y();
  for (const FittableSpline::LinearCoefficient& coeff : coeffs) {
    const int cp_idx = coeff.controlPointIdx;
    f.a[cp_idx * 2 + 1] = coeff.coeff;
    // Because we want a function from control point displacements, not positions.
    f.b += m_spline->controlPointPosition(cp_idx).y() * coeff.coeff;
  }
  m_constraints.push_back(f);
}

void ConstraintSet::constrainControlPoint(int cp_idx, const QLineF& line) {
  assert(cp_idx >= 0 && cp_idx < m_spline->numControlPoints());

  if (line.p1() == line.p2()) {
    constrainControlPoint(cp_idx, line.p1());

    return;
  }

  const double dx = line.p2().x() - line.p1().x();
  const double dy = line.p2().y() - line.p1().y();

  // Lx(cp) = p1.x + t * dx
  // Ly(cp) = p1.y + t * dy
  // Lx(cp) * dy = p1.x * dy + t * dx * dy
  // Ly(cp) * dx = p1.y * dx + t * dx * dy
  // Lx(cp) * dy - Ly(cp) * dx = p1.x * dy - p1.y * dx
  // L(cp) = Lx(cp) * dy - Ly(cp) * dx
  // L(cp) + (p1.y * dx - p1.x * dy) = 0

  LinearFunction f(m_spline->numControlPoints() * 2);
  f.a[cp_idx * 2] = dy;
  f.a[cp_idx * 2 + 1] = -dx;
  f.b = line.p1().y() * dx - line.p1().x() * dy;

  // Make it a function of control point displacements, not control points themselves.
  const QPointF cp(m_spline->controlPointPosition(cp_idx));
  f.b += cp.x() * dy;
  f.b += cp.y() * dx;

  m_constraints.push_back(f);
}  // ConstraintSet::constrainControlPoint

void ConstraintSet::constrainSplinePoint(double t, const QLineF& line) {
  if (line.p1() == line.p2()) {
    constrainSplinePoint(t, line.p1());

    return;
  }

  std::vector<FittableSpline::LinearCoefficient> coeffs;
  m_spline->linearCombinationAt(t, coeffs);

  const double dx = line.p2().x() - line.p1().x();
  const double dy = line.p2().y() - line.p1().y();

  // Lx(cp) = p1.x + t * dx
  // Ly(cp) = p1.y + t * dy
  // Lx(cp) * dy = p1.x * dy + t * dx * dy
  // Ly(cp) * dx = p1.y * dx + t * dx * dy
  // Lx(cp) * dy - Ly(cp) * dx = p1.x * dy - p1.y * dx
  // L(cp) = Lx(cp) * dy - Ly(cp) * dx
  // L(cp) + (p1.y * dx - p1.x * dy) = 0

  LinearFunction f(m_spline->numControlPoints() * 2);
  f.b = line.p1().y() * dx - line.p1().x() * dy;
  for (const FittableSpline::LinearCoefficient& coeff : coeffs) {
    f.a[coeff.controlPointIdx * 2] = coeff.coeff * dy;
    f.a[coeff.controlPointIdx * 2 + 1] = -coeff.coeff * dx;

    // Because we want a function from control point displacements, not positions.
    const QPointF cp(m_spline->controlPointPosition(coeff.controlPointIdx));
    f.b += cp.x() * coeff.coeff * dy - cp.y() * coeff.coeff * dx;
  }
  m_constraints.push_back(f);
}  // ConstraintSet::constrainSplinePoint
}  // namespace spfit