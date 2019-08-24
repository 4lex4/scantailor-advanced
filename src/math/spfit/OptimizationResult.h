// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_SPFIT_OPTIMIZATIONRESULT_H_
#define SCANTAILOR_SPFIT_OPTIMIZATIONRESULT_H_

#include <limits>

namespace spfit {
class OptimizationResult {
 public:
  OptimizationResult(double forceBefore, double forceAfter);

  double forceBefore() const { return m_forceBefore; }

  double forceAfter() const { return m_forceAfter; }

  /**
   * \brief Returns force decrease in percents.
   *
   * Force decrease can theoretically be negative.
   *
   * \note Improvements from different optimization runs can't be compared,
   *       as the absolute force values depend on the number of samples,
   *       which varies from one optimization iteration to another.
   */
  double improvementPercentage() const;

 private:
  double m_forceBefore;
  double m_forceAfter;
};
}  // namespace spfit
#endif
