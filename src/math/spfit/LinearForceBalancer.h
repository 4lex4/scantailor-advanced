// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SPFIT_LINEAR_FORCE_BALANCER_H_
#define SPFIT_LINEAR_FORCE_BALANCER_H_

namespace spfit {
/**
 * \brief Implements one possible strategy of choosing the internal / external force ratio.
 *
 * The strategy is starting with some value and linearly changing it (usually decreasing)
 *  over time.
 */
class LinearForceBalancer {
  // Member-wise copying is OK.
 public:
  /**
   * Sets both the current and the target ratio, so that it doesn't change over time.
   */
  explicit LinearForceBalancer(double internal_external_ratio);

  double currentRatio() const { return m_currentRatio; }

  void setCurrentRatio(double internal_external_ratio);

  double targetRatio() const { return m_targetRatio; }

  void setTargetRatio(double internal_external_ratio);

  /**
   * Sets the number of iterations after which the internal / external force
   * ratio reaches its target value.  This method doesn't change the
   * current ratio.
   */
  void setIterationsToTarget(int iterations);

  double calcInternalForceWeight(double internal_force, double external_force) const;

  /**
   * Returns the current internal / external force ratio, then moves
   * it towards its target value.  After it reaches its target value,
   * further nextIteration() calls will keep returning the same value.
   */
  void nextIteration();

 private:
  void recalcRateOfChange();

  double m_currentRatio;
  double m_targetRatio;
  double m_rateOfChange;
  int m_iterationsToTarget;
};
}  // namespace spfit
#endif  // ifndef SPFIT_LINEAR_FORCE_BALANCER_H_
