// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "Optimizer.h"

#include <boost/foreach.hpp>

#include "MatrixCalc.h"

namespace spfit {
Optimizer::Optimizer(size_t numVars)
    : m_numVars(numVars),
      m_A(numVars, numVars),
      m_b(numVars),
      m_x(numVars),
      m_externalForce(numVars),
      m_internalForce(numVars) {}

void Optimizer::setConstraints(const std::list<LinearFunction>& constraints) {
  const size_t numConstraints = constraints.size();
  const size_t numDimensions = m_numVars + numConstraints;

  MatT<double> A(numDimensions, numDimensions);
  VecT<double> b(numDimensions);
  // Matrix A and vector b will have the following layout:
  // |N N N L L|      |-D|
  // |N N N L L|      |-D|
  // A = |N N N L L|  b = |-D|
  // |C C C 0 0|      |-J|
  // |C C C 0 0|      |-J|
  // N: non-constant part of the gradient of the function we are minimizing.
  // C: non-constant part of constraint functions (one per line).
  // L: coefficients of Lagrange multipliers.  These happen to be equal
  // to the symmetric C values.
  // D: constant part of the gradient of the function we are optimizing.
  // J: constant part of constraint functions.

  auto ctr(constraints.begin());
  for (size_t i = m_numVars; i < numDimensions; ++i, ++ctr) {
    b[i] = -ctr->b;
    for (size_t j = 0; j < m_numVars; ++j) {
      A(i, j) = A(j, i) = ctr->a[j];
    }
  }

  VecT<double>(numDimensions).swap(m_x);
  m_A.swap(A);
  m_b.swap(b);
}  // Optimizer::setConstraints

void Optimizer::addExternalForce(const QuadraticFunction& force) {
  m_externalForce += force;
}

void Optimizer::addExternalForce(const QuadraticFunction& force, const std::vector<int>& sparseMap) {
  const size_t numVars = force.numVars();
  for (size_t i = 0; i < numVars; ++i) {
    const int ii = sparseMap[i];
    for (size_t j = 0; j < numVars; ++j) {
      const int jj = sparseMap[j];
      m_externalForce.A(ii, jj) += force.A(i, j);
    }
    m_externalForce.b[ii] += force.b[i];
  }
  m_externalForce.c += force.c;
}

void Optimizer::addInternalForce(const QuadraticFunction& force) {
  m_internalForce += force;
}

void Optimizer::addInternalForce(const QuadraticFunction& force, const std::vector<int>& sparseMap) {
  const size_t numVars = force.numVars();
  for (size_t i = 0; i < numVars; ++i) {
    const int ii = sparseMap[i];
    for (size_t j = 0; j < numVars; ++j) {
      const int jj = sparseMap[j];
      m_internalForce.A(ii, jj) += force.A(i, j);
    }
    m_internalForce.b[ii] += force.b[i];
  }
  m_internalForce.c += force.c;
}

OptimizationResult Optimizer::optimize(double internalForceWeight) {
  // Note: because we are supposed to reset the forces anyway,
  // we re-use m_internalForce to store the cummulative force.
  m_internalForce *= internalForceWeight;
  m_internalForce += m_externalForce;

  // For the layout of m_A and m_b, see setConstraints()
  const QuadraticFunction::Gradient grad(m_internalForce.gradient());
  for (size_t i = 0; i < m_numVars; ++i) {
    m_b[i] = -grad.b[i];
    for (size_t j = 0; j < m_numVars; ++j) {
      m_A(i, j) = grad.A(i, j);
    }
  }

  const double totalForceBefore = m_internalForce.c;
  DynamicMatrixCalc<double> mc;

  try {
    mc(m_A).solve(mc(m_b)).write(m_x.data());
  } catch (const std::runtime_error&) {
    m_externalForce.reset();
    m_internalForce.reset();
    m_x.fill(0);  // To make undoLastStep() work as expected.
    return OptimizationResult(totalForceBefore, totalForceBefore);
  }

  const double totalForceAfter = m_internalForce.evaluate(m_x.data());
  m_externalForce.reset();  // Now it's finally safe to reset these.
  m_internalForce.reset();
  // The last thing remaining is to adjust constraints,
  // as they depend on the current variables.
  adjustConstraints(1.0);
  return OptimizationResult(totalForceBefore, totalForceAfter);
}  // Optimizer::optimize

void Optimizer::undoLastStep() {
  adjustConstraints(-1.0);
  m_x.fill(0);
}

/**
 * direction == 1 is used for forward adjustment,
 * direction == -1 is used for undoing the last step.
 */
void Optimizer::adjustConstraints(double direction) {
  const size_t numDimensions = m_b.size();
  for (size_t i = m_numVars; i < numDimensions; ++i) {
    // See setConstraints() for more information
    // on the layout of m_A and m_b.
    double c = 0;
    for (size_t j = 0; j < m_numVars; ++j) {
      c += m_A(i, j) * m_x[j];
    }
    m_b[i] -= c * direction;
  }
}

void Optimizer::swap(Optimizer& other) {
  m_A.swap(other.m_A);
  m_b.swap(other.m_b);
  m_x.swap(other.m_x);
  m_externalForce.swap(other.m_externalForce);
  m_internalForce.swap(other.m_internalForce);
  std::swap(m_numVars, other.m_numVars);
}
}  // namespace spfit