// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "PolynomialLine.h"

#include <stdexcept>

#include "MatT.h"
#include "MatrixCalc.h"

namespace imageproc {
void PolynomialLine::validateArguments(const int degree, const int numValues) {
  if (degree < 0) {
    throw std::invalid_argument("PolynomialLine: degree is invalid");
  }
  if (numValues <= 0) {
    throw std::invalid_argument("PolynomialLine: no data points");
  }
}

double PolynomialLine::calcScale(const int numValues) {
  if (numValues <= 1) {
    return 0.0;
  } else {
    return 1.0 / (numValues - 1);
  }
}

void PolynomialLine::doLeastSquares(const VecT<double>& dataPoints, VecT<double>& coeffs) {
  const auto numTerms = static_cast<int>(coeffs.size());
  const auto numValues = static_cast<int>(dataPoints.size());

  // The least squares equation is A^T*A*x = A^T*b
  // We will be building A^T*A and A^T*b incrementally.
  // This allows us not to build matrix A at all.
  MatT<double> AtA(numTerms, numTerms);
  VecT<double> Atb(numTerms);

  // 1, x, x^2, x^3, ...
  VecT<double> powers(numTerms);

  // Pretend that data points are positioned in range of [0, 1].
  const double scale = calcScale(numValues);
  for (int x = 0; x < numValues; ++x) {
    const double position = x * scale;
    const double dataPoint = dataPoints[x];

    // Fill the powers vector.
    double pow = 1.0;
    for (int j = 0; j < numTerms; ++j) {
      powers[j] = pow;
      pow *= position;
    }

    // Update AtA and Atb.
    for (int i = 0; i < numTerms; ++i) {
      const double iVal = powers[i];
      Atb[i] += iVal * dataPoint;
      for (int j = 0; j < numTerms; ++j) {
        const double jVal = powers[j];
        AtA(i, j) += iVal * jVal;
      }
    }
  }

  // In case AtA is rank-deficient, we can usually fix it like this:
  for (int i = 0; i < numTerms; ++i) {
    AtA(i, i) += 1e-5;  // Add a small value to the diagonal.
  }

  try {
    DynamicMatrixCalc<double> mc;
    mc(AtA).solve(mc(Atb)).write(coeffs.data());
  } catch (const std::runtime_error&) {
  }
}  // PolynomialLine::doLeastSquares
}  // namespace imageproc
