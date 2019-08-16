// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "PolynomialLine.h"
#include <stdexcept>
#include "MatT.h"
#include "MatrixCalc.h"

namespace imageproc {
void PolynomialLine::validateArguments(const int degree, const int num_values) {
  if (degree < 0) {
    throw std::invalid_argument("PolynomialLine: degree is invalid");
  }
  if (num_values <= 0) {
    throw std::invalid_argument("PolynomialLine: no data points");
  }
}

double PolynomialLine::calcScale(const int num_values) {
  if (num_values <= 1) {
    return 0.0;
  } else {
    return 1.0 / (num_values - 1);
  }
}

void PolynomialLine::doLeastSquares(const VecT<double>& data_points, VecT<double>& coeffs) {
  const auto num_terms = static_cast<int>(coeffs.size());
  const auto num_values = static_cast<int>(data_points.size());

  // The least squares equation is A^T*A*x = A^T*b
  // We will be building A^T*A and A^T*b incrementally.
  // This allows us not to build matrix A at all.
  MatT<double> AtA(num_terms, num_terms);
  VecT<double> Atb(num_terms);

  // 1, x, x^2, x^3, ...
  VecT<double> powers(num_terms);

  // Pretend that data points are positioned in range of [0, 1].
  const double scale = calcScale(num_values);
  for (int x = 0; x < num_values; ++x) {
    const double position = x * scale;
    const double data_point = data_points[x];

    // Fill the powers vector.
    double pow = 1.0;
    for (int j = 0; j < num_terms; ++j) {
      powers[j] = pow;
      pow *= position;
    }

    // Update AtA and Atb.
    for (int i = 0; i < num_terms; ++i) {
      const double i_val = powers[i];
      Atb[i] += i_val * data_point;
      for (int j = 0; j < num_terms; ++j) {
        const double j_val = powers[j];
        AtA(i, j) += i_val * j_val;
      }
    }
  }

  // In case AtA is rank-deficient, we can usually fix it like this:
  for (int i = 0; i < num_terms; ++i) {
    AtA(i, i) += 1e-5;  // Add a small value to the diagonal.
  }

  try {
    DynamicMatrixCalc<double> mc;
    mc(AtA).solve(mc(Atb)).write(coeffs.data());
  } catch (const std::runtime_error&) {
  }
}  // PolynomialLine::doLeastSquares
}  // namespace imageproc
