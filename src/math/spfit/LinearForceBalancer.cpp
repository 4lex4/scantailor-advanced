// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "LinearForceBalancer.h"
#include <cmath>

namespace spfit {
LinearForceBalancer::LinearForceBalancer(double internalExternalRatio)
    : m_currentRatio(internalExternalRatio),
      m_targetRatio(internalExternalRatio),
      m_rateOfChange(0),
      m_iterationsToTarget(0) {}

void LinearForceBalancer::setCurrentRatio(double internalExternalRatio) {
  m_currentRatio = internalExternalRatio;
  recalcRateOfChange();
}

void LinearForceBalancer::setTargetRatio(double internalExternalRatio) {
  m_targetRatio = internalExternalRatio;
  recalcRateOfChange();
}

void LinearForceBalancer::setIterationsToTarget(int iterations) {
  m_iterationsToTarget = iterations;
  recalcRateOfChange();
}

double LinearForceBalancer::calcInternalForceWeight(double internalForce, double externalForce) const {
  // (internal * lambda) / external = ratio
  // internal * lambda = external * ratio
  double lambda = 0;
  if (std::fabs(internalForce) > 1e-6) {
    lambda = m_currentRatio * externalForce / internalForce;
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