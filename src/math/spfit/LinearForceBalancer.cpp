// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "LinearForceBalancer.h"
#include <cmath>

namespace spfit {
LinearForceBalancer::LinearForceBalancer(double internal_external_ratio)
    : m_currentRatio(internal_external_ratio),
      m_targetRatio(internal_external_ratio),
      m_rateOfChange(0),
      m_iterationsToTarget(0) {}

void LinearForceBalancer::setCurrentRatio(double internal_external_ratio) {
  m_currentRatio = internal_external_ratio;
  recalcRateOfChange();
}

void LinearForceBalancer::setTargetRatio(double internal_external_ratio) {
  m_targetRatio = internal_external_ratio;
  recalcRateOfChange();
}

void LinearForceBalancer::setIterationsToTarget(int iterations) {
  m_iterationsToTarget = iterations;
  recalcRateOfChange();
}

double LinearForceBalancer::calcInternalForceWeight(double internal_force, double external_force) const {
  // (internal * lambda) / external = ratio
  // internal * lambda = external * ratio
  double lambda = 0;
  if (std::fabs(internal_force) > 1e-6) {
    lambda = m_currentRatio * external_force / internal_force;
  }

  return lambda;
}

void LinearForceBalancer::nextIteration() {
  if (m_iterationsToTarget > 0) {
    --m_iterationsToTarget;
    m_currentRatio += m_rateOfChange;
  }
}

void LinearForceBalancer::recalcRateOfChange() {
  if (m_iterationsToTarget <= 0) {
    // Already there.
    m_rateOfChange = 0;
  } else {
    m_rateOfChange = (m_targetRatio - m_currentRatio) / m_iterationsToTarget;
  }
}
}  // namespace spfit