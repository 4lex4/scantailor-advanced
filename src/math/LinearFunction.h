// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef LINEAR_FUNCTION_H_
#define LINEAR_FUNCTION_H_

#include <cstddef>
#include "VecT.h"

/**
 * A linear function from arbitrary number of variables
 * expressed in matrix form:
 * \code
 * F(x) = a^T * x + b
 * \endcode
 */
class LinearFunction {
  // Member-wise copying is OK.
 public:
  VecT<double> a;
  double b;

  /**
   * Constructs a linear function of the given number of variables,
   * initializing everything to zero.
   */
  explicit LinearFunction(size_t num_vars = 0);

  /**
   * Resets everything to zero, so that F(x) = 0
   */
  void reset();

  size_t numVars() const { return a.size(); }

  /**
   * Evaluates a^T * x + b
   */
  double evaluate(const double* x) const;

  void swap(LinearFunction& other);

  LinearFunction& operator+=(const LinearFunction& other);

  LinearFunction& operator*=(double scalar);
};


inline void swap(LinearFunction& f1, LinearFunction& f2) {
  f1.swap(f2);
}

#endif  // ifndef LINEAR_FUNCTION_H_
