// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_MATH_QUADRATICFUNCTION_H_
#define SCANTAILOR_MATH_QUADRATICFUNCTION_H_

#include <cstddef>
#include "MatT.h"
#include "VecT.h"

/**
 * A quadratic function from arbitrary number of variables
 * expressed in matrix form:
 * \code
 * F(x) = x^T * A * x + b^T * x + c
 * \endcode
 * With N being the number of variables, we have:\n
 * x: vector of N variables.\n
 * A: NxN matrix of coefficients.\n
 * b: vector of N coefficients.\n
 * c: constant component.\n
 */
class QuadraticFunction {
  // Member-wise copying is OK.
 public:
  /**
   * Quadratic function's gradient can be written in matrix form as:
   * \code
   * nabla F(x) = A * x + b
   * \endcode
   */
  class Gradient {
   public:
    MatT<double> A;
    VecT<double> b;
  };


  /**
   * Matrix A has column-major data storage, so that it can be used with MatrixCalc.
   */
  MatT<double> A;
  VecT<double> b;
  double c;

  /**
   * Constructs a quadratic functiono of the given number of variables,
   * initializing everything to zero.
   */
  explicit QuadraticFunction(size_t numVars = 0);

  /**
   * Resets everything to zero, so that F(x) = 0
   */
  void reset();

  size_t numVars() const { return b.size(); }

  /**
   * Evaluates x^T * A * x + b^T * x + c
   */
  double evaluate(const double* x) const;

  Gradient gradient() const;

  /**
   * f(x) is our function.  This method will replace f(x) with g(x) so that
   * g(x) = f(x + translation)
   */
  void recalcForTranslatedArguments(const double* translation);

  void swap(QuadraticFunction& other);

  QuadraticFunction& operator+=(const QuadraticFunction& other);

  QuadraticFunction& operator*=(double scalar);
};


inline void swap(QuadraticFunction& f1, QuadraticFunction& f2) {
  f1.swap(f2);
}

#endif  // ifndef SCANTAILOR_MATH_QUADRATICFUNCTION_H_
