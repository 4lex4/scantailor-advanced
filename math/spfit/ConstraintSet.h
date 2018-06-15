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
  const FittableSpline* m_pSpline;
  std::list<LinearFunction> m_constraints;
};
}  // namespace spfit
#endif  // ifndef SPFIT_CONSTRAINT_SET_H_
