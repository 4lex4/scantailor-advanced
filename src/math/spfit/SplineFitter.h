// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_SPFIT_SPLINEFITTER_H_
#define SCANTAILOR_SPFIT_SPLINEFITTER_H_

#include <vector>

#include "FittableSpline.h"
#include "NonCopyable.h"
#include "Optimizer.h"
#include "VecNT.h"

namespace spfit {
class ConstraintSet;
class ModelShape;

struct SqDistApproximant;

class OptimizationResult;

class SplineFitter {
  DECLARE_NON_COPYABLE(SplineFitter)

 public:
  explicit SplineFitter(FittableSpline* spline);

  /**
   * To be called after adding / moving / removing any of spline's control points.
   * This will reset the optimizer, which means the current set of constraints
   * is lost.  Any forces accumulated since the last optimize() call are lost as well.
   */
  void splineModified();

  void setConstraints(const ConstraintSet& constraints);

  void setSamplingParams(const FittableSpline::SamplingParams& samplingParams);

  void addAttractionForce(const Vec2d& splinePoint,
                          const std::vector<FittableSpline::LinearCoefficient>& coeffs,
                          const SqDistApproximant& sqdistApprox);

  void addAttractionForces(const ModelShape& modelShape, double fromT = 0.0, double toT = 1.0);

  void addExternalForce(const QuadraticFunction& force);

  void addExternalForce(const QuadraticFunction& force, const std::vector<int>& sparseMap);

  void addInternalForce(const QuadraticFunction& force);

  void addInternalForce(const QuadraticFunction& force, const std::vector<int>& sparceMap);

  /** \see Optimizer::externalForce() */
  double externalForce() const { return m_optimizer.externalForce(); }

  /** \see Optimizer::internalForce() */
  double internalForce() const { return m_optimizer.internalForce(); }

  OptimizationResult optimize(double internalForceWeight);

  void undoLastStep();

 private:
  FittableSpline* m_spline;
  Optimizer m_optimizer;
  FittableSpline::SamplingParams m_samplingParams;
  std::vector<double> m_tempVars;
  std::vector<int> m_tempSparseMap;
  std::vector<FittableSpline::LinearCoefficient> m_tempCoeffs;
};
}  // namespace spfit
#endif  // ifndef SCANTAILOR_SPFIT_SPLINEFITTER_H_
