// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "SplineFitter.h"
#include <boost/foreach.hpp>
#include "ConstraintSet.h"
#include "ModelShape.h"

namespace spfit {
SplineFitter::SplineFitter(FittableSpline* spline) : m_spline(spline), m_optimizer(spline->numControlPoints() * 2) {
  // Each control point is a pair of (x, y) varaiables.
}

void SplineFitter::splineModified() {
  Optimizer(m_spline->numControlPoints() * 2).swap(m_optimizer);
}

void SplineFitter::setConstraints(const ConstraintSet& constraints) {
  m_optimizer.setConstraints(constraints.constraints());
}

void SplineFitter::setSamplingParams(const FittableSpline::SamplingParams& params) {
  m_samplingParams = params;
}

void SplineFitter::addAttractionForce(const Vec2d& splinePoint,
                                      const std::vector<FittableSpline::LinearCoefficient>& coeffs,
                                      const SqDistApproximant& sqdistApprox) {
  const auto numCoeffs = static_cast<int>(coeffs.size());
  const int numVars = numCoeffs * 2;
  QuadraticFunction f(numVars);

  // Right now we basically have F(x) = Q(L(x)),
  // where Q is a quadratic function represented by sqdistApprox,
  // and L is a vector of linear combinations of control points,
  // represented by coeffs.  L[0] = is a linear combination of x coordinats
  // of control points and L[1] is a linear combination of y coordinates of
  // control points.  What we are after is F(x) = Q(x), that is a quadratic
  // function of control points.  We consider control points to be a flat
  // vector of variables, with the following layout: [x0 y0 x1 y1 x2 y2 ...]

  // First deal with the quadratic portion of our function.
  for (int i = 0; i < 2; ++i) {
    for (int j = 0; j < 2; ++j) {
      // Here we have Li * Aij * Lj, which gives us a product of
      // two linear functions times a constant.
      // L0 is a linear combination of x components: L0 = [c1 0  c2  0 ...]
      // L1 is a linear combination of y components: L1 = [ 0 c1  0 c2 ...]
      // Note that the same coefficients are indeed present in both L0 and L1.
      const double a = sqdistApprox.A(i, j);

      // Now let's multiply Li by Lj
      for (int m = 0; m < numCoeffs; ++m) {
        const double c1 = coeffs[m].coeff;
        const int Li_idx = m * 2 + i;
        for (int n = 0; n < numCoeffs; ++n) {
          const double c2 = coeffs[n].coeff;
          const int Lj_idx = n * 2 + j;
          f.A(Li_idx, Lj_idx) += a * c1 * c2;
        }
      }
    }
  }

  // Moving on to the linear part of the function.
  for (int i = 0; i < 2; ++i) {
    // Here we have Li * Bi, that is a linear function times a constant.
    const double b = sqdistApprox.b[i];
    for (int m = 0; m < numCoeffs; ++m) {
      const int Li_idx = m * 2 + i;
      f.b[Li_idx] += b * coeffs[m].coeff;
    }
  }

  // The constant part is easy.
  f.c = sqdistApprox.c;
  // What we've got at this point is a function of control point positions.
  // What we need however, is a function from control point displacements.
  m_tempVars.resize(numVars);
  for (int i = 0; i < numCoeffs; ++i) {
    const int cpIdx = coeffs[i].controlPointIdx;
    const QPointF cp(m_spline->controlPointPosition(cpIdx));
    m_tempVars[i * 2] = cp.x();
    m_tempVars[i * 2 + 1] = cp.y();
  }
  f.recalcForTranslatedArguments(numVars ? &m_tempVars[0] : nullptr);
  // What remains is a mapping from the reduced set of variables to the full set.
  m_tempSparseMap.resize(numVars);
  for (int i = 0; i < numCoeffs; ++i) {
    m_tempSparseMap[i * 2] = coeffs[i].controlPointIdx * 2;
    m_tempSparseMap[i * 2 + 1] = coeffs[i].controlPointIdx * 2 + 1;
  }

  m_optimizer.addExternalForce(f, m_tempSparseMap);
}  // SplineFitter::addAttractionForce

void SplineFitter::addAttractionForces(const ModelShape& modelShape, double fromT, double toT) {
  auto sampleProcessor = [this, &modelShape](const QPointF& pt, double t, FittableSpline::SampleFlags flags) {
    m_spline->linearCombinationAt(t, m_tempCoeffs);
    const SqDistApproximant approx(modelShape.localSqDistApproximant(pt, flags));
    addAttractionForce(pt, m_tempCoeffs, approx);
  };

  m_spline->sample(ProxyFunction<decltype(sampleProcessor), void, const QPointF&, double, FittableSpline::SampleFlags>(
                       sampleProcessor),
                   m_samplingParams, fromT, toT);
}

void SplineFitter::addExternalForce(const QuadraticFunction& force) {
  m_optimizer.addExternalForce(force);
}

void SplineFitter::addExternalForce(const QuadraticFunction& force, const std::vector<int>& sparseMap) {
  m_optimizer.addExternalForce(force, sparseMap);
}

void SplineFitter::addInternalForce(const QuadraticFunction& force) {
  m_optimizer.addInternalForce(force);
}

void SplineFitter::addInternalForce(const QuadraticFunction& force, const std::vector<int>& sparseMap) {
  m_optimizer.addInternalForce(force, sparseMap);
}

OptimizationResult SplineFitter::optimize(double internalForceWeight) {
  const OptimizationResult res(m_optimizer.optimize(internalForceWeight));

  const int numControlPoints = m_spline->numControlPoints();
  for (int i = 0; i < numControlPoints; ++i) {
    const Vec2d delta(m_optimizer.displacementVector() + i * 2);
    m_spline->moveControlPoint(i, m_spline->controlPointPosition(i) + delta);
  }

  return res;
}

void SplineFitter::undoLastStep() {
  const int numControlPoints = m_spline->numControlPoints();
  for (int i = 0; i < numControlPoints; ++i) {
    const Vec2d delta(m_optimizer.displacementVector() + i * 2);
    m_spline->moveControlPoint(i, m_spline->controlPointPosition(i) - delta);
  }

  m_optimizer.undoLastStep();  // Zeroes the displacement vector among other things.
}
}  // namespace spfit