// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "OptimizationResult.h"
#include <algorithm>

namespace spfit {
OptimizationResult::OptimizationResult(double forceBefore, double forceAfter)
    : m_forceBefore(std::max<double>(forceBefore, 0)), m_forceAfter(std::max<double>(forceAfter, 0)) {
  // In theory, these distances can't be negative, but in practice they can.
  // We are going to treat negative ones as they are zeros.
}

double OptimizationResult::improvementPercentage() const {
  double improvement = m_forceBefore - m_forceAfter;
  improvement /= (m_forceBefore + std::numeric_limits<double>::epsilon());

  return improvement * 100;  // Convert to percents.
}
}  // namespace spfit
