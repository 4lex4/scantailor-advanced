// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "Function.h"
#include <algorithm>

namespace adiff {
// Note:
// 1. u in this file has the same meaning as in [1].
// 2. sparseMap(i, j) corresponds to l(i, j) in [1].

Function<2>::Function(size_t numNonZeroVars) : value(), firstDerivs(numNonZeroVars), secondDerivs(numNonZeroVars) {}

Function<2>::Function(const SparseMap<2>& sparseMap)
    : value(), firstDerivs(sparseMap.numNonZeroElements()), secondDerivs(sparseMap.numNonZeroElements()) {}

Function<2>::Function(size_t argIdx, double val, const SparseMap<2>& sparseMap)
    : value(val), firstDerivs(sparseMap.numNonZeroElements()), secondDerivs(sparseMap.numNonZeroElements()) {
  // An argument Xi is represented as a function:
  // f(X1, X2, ..., Xi, ...) = Xi
  // Derivatives are calculated accordingly.

  const size_t numVars = sparseMap.numVars();

  // argIdx row
  for (size_t i = 0; i < numVars; ++i) {
    const size_t u = sparseMap.nonZeroElementIdx(argIdx, i);
    if (u != sparseMap.ZERO_ELEMENT) {
      firstDerivs[u] = 1.0;
    }
  }
  // argIdx column
  for (size_t i = 0; i < numVars; ++i) {
    const size_t u = sparseMap.nonZeroElementIdx(i, argIdx);
    if (u != sparseMap.ZERO_ELEMENT) {
      firstDerivs[u] = 1.0;
    }
  }
}

VecT<double> Function<2>::gradient(const SparseMap<2>& sparseMap) const {
  const size_t numVars = sparseMap.numVars();
  VecT<double> grad(numVars);

  for (size_t i = 0; i < numVars; ++i) {
    const size_t u = sparseMap.nonZeroElementIdx(i, i);
    if (u != sparseMap.ZERO_ELEMENT) {
      grad[i] = firstDerivs[u];
    }
  }

  return grad;
}

MatT<double> Function<2>::hessian(const SparseMap<2>& sparseMap) const {
  const size_t numVars = sparseMap.numVars();
  MatT<double> hess(numVars, numVars);

  for (size_t i = 0; i < numVars; ++i) {
    for (size_t j = 0; j < numVars; ++j) {
      double Fij = 0;
      const size_t ij = sparseMap.nonZeroElementIdx(i, j);
      if (ij != sparseMap.ZERO_ELEMENT) {
        if (i == j) {
          Fij = secondDerivs[ij];
        } else {
          const size_t ii = sparseMap.nonZeroElementIdx(i, i);
          const size_t jj = sparseMap.nonZeroElementIdx(j, j);
          assert(ii != sparseMap.ZERO_ELEMENT && jj != sparseMap.ZERO_ELEMENT);
          Fij = 0.5 * (secondDerivs[ij] - (secondDerivs[ii] + secondDerivs[jj]));
        }
      }
      hess(i, j) = Fij;
    }
  }

  return hess;
}

void Function<2>::swap(Function<2>& other) {
  std::swap(value, other.value);
  firstDerivs.swap(other.firstDerivs);
  secondDerivs.swap(other.secondDerivs);
}

Function<2>& Function<2>::operator+=(const Function<2>& other) {
  const size_t p = firstDerivs.size();
  assert(secondDerivs.size() == p);
  assert(other.firstDerivs.size() == p);
  assert(other.secondDerivs.size() == p);

  value += other.value;

  for (size_t u = 0; u < p; ++u) {
    firstDerivs[u] += other.firstDerivs[u];
    secondDerivs[u] += other.secondDerivs[u];
  }

  return *this;
}

Function<2>& Function<2>::operator-=(const Function<2>& other) {
  const size_t p = firstDerivs.size();
  assert(secondDerivs.size() == p);
  assert(other.firstDerivs.size() == p);
  assert(other.secondDerivs.size() == p);

  value -= other.value;

  for (size_t u = 0; u < p; ++u) {
    firstDerivs[u] -= other.firstDerivs[u];
    secondDerivs[u] -= other.secondDerivs[u];
  }

  return *this;
}

Function<2>& Function<2>::operator*=(double scalar) {
  const size_t p = firstDerivs.size();
  value *= scalar;

  for (size_t u = 0; u < p; ++u) {
    firstDerivs[u] *= scalar;
  }

  return *this;
}

Function<2> operator+(const Function<2>& f1, const Function<2>& f2) {
  const size_t p = f1.firstDerivs.size();
  assert(f1.secondDerivs.size() == p);
  assert(f2.firstDerivs.size() == p);
  assert(f2.secondDerivs.size() == p);

  Function<2> res(p);
  res.value = f1.value + f2.value;

  for (size_t u = 0; u < p; ++u) {
    res.firstDerivs[u] = f1.firstDerivs[u] + f2.firstDerivs[u];
    res.secondDerivs[u] = f1.secondDerivs[u] + f2.secondDerivs[u];
  }

  return res;
}

Function<2> operator-(const Function<2>& f1, const Function<2>& f2) {
  const size_t p = f1.firstDerivs.size();
  assert(f1.secondDerivs.size() == p);
  assert(f2.firstDerivs.size() == p);
  assert(f2.secondDerivs.size() == p);

  Function<2> res(p);
  res.value = f1.value - f2.value;

  for (size_t u = 0; u < p; ++u) {
    res.firstDerivs[u] = f1.firstDerivs[u] - f2.firstDerivs[u];
    res.secondDerivs[u] = f1.secondDerivs[u] - f2.secondDerivs[u];
  }

  return res;
}

Function<2> operator*(const Function<2>& f1, const Function<2>& f2) {
  const size_t p = f1.firstDerivs.size();
  assert(f1.secondDerivs.size() == p);
  assert(f2.firstDerivs.size() == p);
  assert(f2.secondDerivs.size() == p);

  Function<2> res(p);
  res.value = f1.value * f2.value;

  for (size_t u = 0; u < p; ++u) {
    res.firstDerivs[u] = f1.firstDerivs[u] * f2.value + f1.value * f2.firstDerivs[u];
    res.secondDerivs[u]
        = f1.secondDerivs[u] * f2.value + 2.0 * f1.firstDerivs[u] * f2.firstDerivs[u] + f1.value * f2.secondDerivs[u];
  }

  return res;
}

Function<2> operator*(const Function<2>& f, double scalar) {
  Function<2> res(f);
  res *= scalar;

  return res;
}

Function<2> operator*(double scalar, const Function<2>& f) {
  Function<2> res(f);
  res *= scalar;

  return res;
}

Function<2> operator/(const Function<2>& num, const Function<2>& den) {
  const size_t p = num.firstDerivs.size();
  assert(num.secondDerivs.size() == p);
  assert(den.firstDerivs.size() == p);
  assert(den.secondDerivs.size() == p);

  Function<2> res(p);
  res.value = num.value / den.value;

  const double den2 = den.value * den.value;
  const double den4 = den2 * den2;

  for (size_t u = 0; u < p; ++u) {
    // Derivative of: (num.value / den.value)
    const double d1 = num.firstDerivs[u] * den.value - num.value * den.firstDerivs[u];
    res.firstDerivs[u] = d1 / den2;

    // Derivative of: (num.firstDerivs[u] * den.value - num.value * den.firstDerivs[u])
    const double d2 = num.secondDerivs[u] * den.value - num.value * den.secondDerivs[u];

    // Derivative of: den2
    const double d3 = 2.0 * den.value * den.firstDerivs[u];

    // Derivative of: (d1 / den2)
    res.secondDerivs[u] = (d2 * den2 - d1 * d3) / den4;
  }

  return res;
}
}  // namespace adiff