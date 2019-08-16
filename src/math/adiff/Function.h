// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef ADIFF_FUNCTION_H_
#define ADIFF_FUNCTION_H_

#include <cstddef>
#include "MatT.h"
#include "SparseMap.h"
#include "VecT.h"

namespace adiff {
/**
 * \brief Represents a multivariable function and its derivatives.
 *
 * It would be nice to have a generic version,
 * but for now it's only specialized for ORD == 2.
 */
template <int ORD>
class Function;

template <>
class Function<2> {
  // Member-wise copying is OK.
 public:
  /** The value of the function. */
  double value;

  /**
   * First directional derivatives in the direction
   * of u = i + j for every non-zero Hessian element at i, j.
   */
  VecT<double> firstDerivs;

  /**
   * Second directional derivatives in the direction
   * of u = i + j for every non-zero Hessian element at i, j.
   */
  VecT<double> secondDerivs;

  /**
   * Constructs the "f(x1, x2, ...) = 0" function.
   */
  explicit Function(size_t num_non_zero_vars);

  /**
   * Constructs the "f(x1, x2, ...) = 0" function.
   */
  explicit Function(const SparseMap<2>& sparse_map);

  /**
   * Constructs a function representing an argument.
   *
   * \param arg_idx Argument number.
   * \param val Argument value.
   * \param sparse_map Tells which derivatives to compute.
   */
  Function(size_t arg_idx, double val, const SparseMap<2>& sparse_map);

  VecT<double> gradient(const SparseMap<2>& sparse_map) const;

  MatT<double> hessian(const SparseMap<2>& sparse_map) const;

  void swap(Function& other);

  Function& operator+=(const Function& other);

  Function& operator-=(const Function& other);

  Function& operator*=(double scalar);
};


inline void swap(Function<2>& f1, Function<2>& f2) {
  f1.swap(f2);
}

Function<2> operator+(const Function<2>& f1, const Function<2>& f2);

Function<2> operator-(const Function<2>& f1, const Function<2>& f2);

Function<2> operator*(const Function<2>& f1, const Function<2>& f2);

Function<2> operator*(const Function<2>& f, double scalar);

Function<2> operator*(double scalar, const Function<2>& f);

Function<2> operator/(const Function<2>& num, const Function<2>& den);
}  // namespace adiff
#endif  // ifndef ADIFF_FUNCTION_H_
