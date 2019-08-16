// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SPFIT_CONSTRAINT_SET_H_
#define SPFIT_CONSTRAINT_SET_H_

#include <QLineF>
#include <QPointF>
#include <cstddef>
#include <list>
#include "LinearFunction.h"

namespace spfit {
class FittableSpline;

class ConstraintSet {
  // Member-wise copying is OK.
 public:
  explicit ConstraintSet(const FittableSpline* spline);

  const std::list<LinearFunction>& constraints() const { return m_constraints; }

  void constrainControlPoint(int cp_idx, const QPointF& pos);

  void constrainControlPoint(int cp_idx, const QLineF& line);

  void constrainSplinePoint(double t, const QPointF& pos);

  void constrainSplinePoint(double t, const QLineF& line);

 private:
  const FittableSpline* m_spline;
  std::list<LinearFunction> m_constraints;
};
}  // namespace spfit
#endif  // ifndef SPFIT_CONSTRAINT_SET_H_
