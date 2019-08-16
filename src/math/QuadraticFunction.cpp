// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "QuadraticFunction.h"
#include <algorithm>

QuadraticFunction::QuadraticFunction(size_t num_vars) : A(num_vars, num_vars), b(num_vars), c(0) {}

void QuadraticFunction::reset() {
  A.fill(0);
  b.fill(0);
  c = 0;
}

double QuadraticFunction::evaluate(const double* x) const {
  const size_t num_vars = numVars();

  double sum = c;
  for (size_t i = 0; i < num_vars; ++i) {
    sum += b[i] * x[i];
    for (size_t j = 0; j < num_vars; ++j) {
      sum += x[i] * x[j] * A(i, j);
    }
  }

  return sum;
}

QuadraticFunction::Gradient QuadraticFunction::gradient() const {
  const size_t num_vars = numVars();
  Gradient grad;

  MatT<double>(num_vars, num_vars).swap(grad.A);
  for (size_t i = 0; i < num_vars; ++i) {
    for (size_t j = 0; j < num_vars; ++j) {
      grad.A(i, j) = A(i, j) + A(j, i);
    }
  }

  grad.b = b;

  return grad;
}

void QuadraticFunction::recalcForTranslatedArguments(const double* translation) {
  const size_t num_vars = numVars();

  for (size_t i = 0; i < num_vars; ++i) {
    // Bi * (Xi + Ti) = Bi * Xi + Bi * Ti
    c += b[i] * translation[i];
  }

  for (size_t i = 0; i < num_vars; ++i) {
    for (size_t j = 0; j < num_vars; ++j) {
      // (Xi + Ti)*Aij*(Xj + Tj) = Xi*Aij*Xj + Aij*Tj*Xi + Aij*Ti*Xj + Aij*Ti*Tj
      const double a = A(i, j);
      b[i] += a * translation[j];
      b[j] += a * translation[i];
      c += a * translation[i] * translation[j];
    }
  }
}

void QuadraticFunction::swap(QuadraticFunction& other) {
  A.swap(other.A);
  b.swap(other.b);
  std::swap(c, other.c);
}

QuadraticFunction& QuadraticFunction::operator+=(const QuadraticFunction& other) {
  A += other.A;
  b += other.b;
  c += other.c;

  return *this;
}

QuadraticFunction& QuadraticFunction::operator*=(double scalar) {
  A *= scalar;
  b *= scalar;
  c *= scalar;

  return *this;
}
