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

#ifndef SPFIT_OPTIMIZER_H_
#define SPFIT_OPTIMIZER_H_

#include <list>
#include <vector>
#include "FittableSpline.h"
#include "LinearFunction.h"
#include "MatT.h"
#include "OptimizationResult.h"
#include "QuadraticFunction.h"
#include "SqDistApproximant.h"
#include "VecNT.h"
#include "VecT.h"
#include "VirtualFunction.h"

namespace spfit {
class Optimizer {
  // Member-wise copying is OK.
 public:
  explicit Optimizer(size_t num_vars = 0);

  /**
   * Sets linear constraints in the form of b^T * x + c = 0
   * Note that x in the above formula is not a vector of coordinates
   * but a vector of their displacements.  That is, the constraints
   * to be passed here depend on the current positions of control points.
   * That doesn't mean you have to provide updated constraints
   * on very iteration though, as optimize() will update them for you.
   */
  void setConstraints(const std::list<LinearFunction>& constraints);

  void addExternalForce(const QuadraticFunction& force);

  void addExternalForce(const QuadraticFunction& force, const std::vector<int>& sparse_map);

  void addInternalForce(const QuadraticFunction& force);

  void addInternalForce(const QuadraticFunction& force, const std::vector<int>& sparse_map);

  size_t numVars() const { return m_numVars; }

  /**
   * Get the external force accumulated from calls to addAttractionForce().
   * Note that optimize() will reset all forces.
   */
  double externalForce() const { return m_externalForce.c; }

  /**
   * Get the internal force accumulated from calls to addInternalForce().
   * Note that optimize() will reset all forces.
   */
  double internalForce() const { return m_internalForce.c; }

  OptimizationResult optimize(double internal_external_ratio);

  const double* displacementVector() const { return m_x.data(); }

  /**
   * Rolls back the very last adjustment to constraints done by optimize()
   * and sets the displacement vector to all zeros.
   */
  void undoLastStep();

  void swap(Optimizer& other);

 private:
  void adjustConstraints(double direction);

  size_t m_numVars;
  MatT<double> m_A;
  VecT<double> m_b;
  VecT<double> m_x;
  QuadraticFunction m_externalForce;
  QuadraticFunction m_internalForce;
};


inline void swap(Optimizer& o1, Optimizer& o2) {
  o1.swap(o2);
}
}  // namespace spfit
#endif  // ifndef SPFIT_OPTIMIZER_H_
