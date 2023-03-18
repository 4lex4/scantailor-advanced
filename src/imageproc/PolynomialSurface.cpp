// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "PolynomialSurface.h"

#include <QDebug>
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <stdexcept>

#include "AlignedArray.h"
#include "BinaryImage.h"
#include "BitOps.h"
#include "GrayImage.h"
#include "Grayscale.h"
#include "MatT.h"
#include "MatrixCalc.h"
#include "VecT.h"

namespace imageproc {
PolynomialSurface::PolynomialSurface(const int horDegree, const int vertDegree, const GrayImage& src)
    : m_horDegree(horDegree), m_vertDegree(vertDegree) {
  // Note: m_horDegree and m_vertDegree may still change!

  if (horDegree < 0) {
    throw std::invalid_argument("PolynomialSurface: horizontal degree is invalid");
  }
  if (vertDegree < 0) {
    throw std::invalid_argument("PolynomialSurface: vertical degree is invalid");
  }

  const int numDataPoints = src.width() * src.height();
  if (numDataPoints == 0) {
    m_horDegree = 0;
    m_vertDegree = 0;
    VecT<double>(1, 0.0).swap(m_coeffs);
    return;
  }

  maybeReduceDegrees(numDataPoints);

  const int numTerms = calcNumTerms();
  VecT<double>(numTerms, 0.0).swap(m_coeffs);

  // The least squares equation is A^T*A*x = A^T*b
  // We will be building A^T*A and A^T*b incrementally.
  // This allows us not to build matrix A at all.
  MatT<double> AtA(numTerms, numTerms);
  VecT<double> Atb(numTerms);
  prepareDataForLeastSquares(src, AtA, Atb, m_horDegree, m_vertDegree);

  fixSquareMatrixRankDeficiency(AtA);

  try {
    DynamicMatrixCalc<double> mc;
    mc(AtA).solve(mc(Atb)).write(m_coeffs.data());
  } catch (const std::runtime_error&) {
  }
}

PolynomialSurface::PolynomialSurface(const int horDegree,
                                     const int vertDegree,
                                     const GrayImage& src,
                                     const BinaryImage& mask)
    : m_horDegree(horDegree), m_vertDegree(vertDegree) {
  // Note: m_horDegree and m_vertDegree may still change!

  if (horDegree < 0) {
    throw std::invalid_argument("PolynomialSurface: horizontal degree is invalid");
  }
  if (vertDegree < 0) {
    throw std::invalid_argument("PolynomialSurface: vertical degree is invalid");
  }
  if (src.size() != mask.size()) {
    throw std::invalid_argument("PolynomialSurface: image and mask have different sizes");
  }

  const int numDataPoints = mask.countBlackPixels();
  if (numDataPoints == 0) {
    m_horDegree = 0;
    m_vertDegree = 0;
    VecT<double>(1, 0.0).swap(m_coeffs);
    return;
  }

  maybeReduceDegrees(numDataPoints);

  const int numTerms = calcNumTerms();
  VecT<double>(numTerms, 0.0).swap(m_coeffs);

  // The least squares equation is A^T*A*x = A^T*b
  // We will be building A^T*A and A^T*b incrementally.
  // This allows us not to build matrix A at all.
  MatT<double> AtA(numTerms, numTerms);
  VecT<double> Atb(numTerms);
  prepareDataForLeastSquares(src, mask, AtA, Atb, m_horDegree, m_vertDegree);

  fixSquareMatrixRankDeficiency(AtA);

  try {
    DynamicMatrixCalc<double> mc;
    mc(AtA).solve(mc(Atb)).write(m_coeffs.data());
  } catch (const std::runtime_error&) {
  }
}

GrayImage PolynomialSurface::render(const QSize& size) const {
  if (size.isEmpty()) {
    return GrayImage();
  }

  GrayImage image(size);
  const int width = size.width();
  const int height = size.height();
  unsigned char* line = image.data();
  const int bpl = image.stride();
  const auto numCoeffs = static_cast<int>(m_coeffs.size());

  // Pretend that both x and y positions of pixels
  // lie in range of [0, 1].
  const double xscale = calcScale(width);
  const double yscale = calcScale(height);

  AlignedArray<float, 4> vertMatrix(numCoeffs * height);
  float* out = &vertMatrix[0];
  for (int y = 0; y < height; ++y) {
    const double yAdjusted = y * yscale;
    double pow = 1.0;
    int pos = 0;
    for (int i = 0; i <= m_vertDegree; ++i) {
      for (int j = 0; j <= m_horDegree; ++j, ++pos, ++out) {
        *out = static_cast<float>(m_coeffs[pos] * pow);
      }
      pow *= yAdjusted;
    }
  }

  AlignedArray<float, 4> horMatrix(numCoeffs * width);
  out = &horMatrix[0];
  for (int x = 0; x < width; ++x) {
    const double xAdjusted = x * xscale;
    for (int i = 0; i <= m_vertDegree; ++i) {
      double pow = 1.0;
      for (int j = 0; j <= m_horDegree; ++j, ++out) {
        *out = static_cast<float>(pow);
        pow *= xAdjusted;
      }
    }
  }

  const float* vertLine = &vertMatrix[0];
  for (int y = 0; y < height; ++y, line += bpl, vertLine += numCoeffs) {
    const float* horLine = &horMatrix[0];
    for (int x = 0; x < width; ++x, horLine += numCoeffs) {
      float sum = 0.5f / 255.0f;  // for rounding purposes.
      for (int i = 0; i < numCoeffs; ++i) {
        sum += horLine[i] * vertLine[i];
      }
      const auto isum = (int) (sum * 255.0);
      line[x] = static_cast<unsigned char>(qBound(0, isum, 255));
    }
  }
  return image;
}  // PolynomialSurface::render

void PolynomialSurface::maybeReduceDegrees(const int numDataPoints) {
  assert(numDataPoints > 0);

  while (numDataPoints < calcNumTerms()) {
    if (m_horDegree > m_vertDegree) {
      --m_horDegree;
    } else {
      --m_vertDegree;
    }
  }
}

int PolynomialSurface::calcNumTerms() const {
  return (m_horDegree + 1) * (m_vertDegree + 1);
}

double PolynomialSurface::calcScale(const int dimension) {
  if (dimension <= 1) {
    return 0.0;
  } else {
    return 1.0 / (dimension - 1);
  }
}

void PolynomialSurface::prepareDataForLeastSquares(const GrayImage& image,
                                                   MatT<double>& AtA,
                                                   VecT<double>& Atb,
                                                   const int hDegree,
                                                   const int vDegree) {
  double* const AtA_data = AtA.data();
  double* const Atb_data = Atb.data();

  const int width = image.width();
  const int height = image.height();
  const auto numTerms = static_cast<int>(Atb.size());

  const uint8_t* line = image.data();
  const int stride = image.stride();

  // Pretend that both x and y positions of pixels
  // lie in range of [0, 1].
  const double xscale = calcScale(width);
  const double yscale = calcScale(height);

  // To force data samples into [0, 1] range.
  const double dataScale = 1.0 / 255.0;

  // 1, y, y^2, y^3, ...
  VecT<double> yPowers(vDegree + 1);  // Initialized to 0.

  // Same as yPowers, except yPowers correspond to a given y,
  // while xPowers are computed for all possible x values.
  MatT<double> xPowers(hDegree + 1, width);  // Initialized to 0.
  for (int x = 0; x < width; ++x) {
    const double xAdjusted = xscale * x;
    double xPower = 1.0;
    for (int i = 0; i <= hDegree; ++i) {
      xPowers(i, x) = xPower;
      xPower *= xAdjusted;
    }
  }

  VecT<double> fullPowers(numTerms);

  for (int y = 0; y < height; ++y, line += stride) {
    const double yAdjusted = yscale * y;

    double yPower = 1.0;
    for (int i = 0; i <= vDegree; ++i) {
      yPowers[i] = yPower;
      yPower *= yAdjusted;
    }

    for (int x = 0; x < width; ++x) {
      const double dataPoint = dataScale * line[x];

      int pos = 0;
      for (int i = 0; i <= vDegree; ++i) {
        for (int j = 0; j <= hDegree; ++j, ++pos) {
          fullPowers[pos] = yPowers[i] * xPowers(j, x);
        }
      }

      double* p_AtA = AtA_data;
      for (int i = 0; i < numTerms; ++i) {
        const double iVal = fullPowers[i];
        Atb_data[i] += iVal * dataPoint;

        for (int j = 0; j < numTerms; ++j) {
          const double jVal = fullPowers[j];
          *p_AtA += iVal * jVal;
          ++p_AtA;
        }
      }
    }
  }
}  // PolynomialSurface::prepareDataForLeastSquares

void PolynomialSurface::prepareDataForLeastSquares(const GrayImage& image,
                                                   const BinaryImage& mask,
                                                   MatT<double>& AtA,
                                                   VecT<double>& Atb,
                                                   const int hDegree,
                                                   const int vDegree) {
  double* const AtA_data = AtA.data();
  double* const Atb_data = Atb.data();

  const int width = image.width();
  const int height = image.height();
  const auto numTerms = static_cast<int>(Atb.size());

  const uint8_t* imageLine = image.data();
  const int imageStride = image.stride();

  const uint32_t* maskLine = mask.data();
  const int maskStride = mask.wordsPerLine();

  // Pretend that both x and y positions of pixels
  // lie in range of [0, 1].
  const double xscale = calcScale(width);
  const double yscale = calcScale(height);

  // To force data samples into [0, 1] range.
  const double dataScale = 1.0 / 255.0;

  // 1, y, y^2, y^3, ...
  VecT<double> yPowers(vDegree + 1);  // Initialized to 0.

  // Same as yPowers, except yPowers correspond to a given y,
  // while xPowers are computed for all possible x values.
  MatT<double> xPowers(hDegree + 1, width);  // Initialized to 0.
  for (int x = 0; x < width; ++x) {
    const double xAdjusted = xscale * x;
    double xPower = 1.0;
    for (int i = 0; i <= hDegree; ++i) {
      xPowers(i, x) = xPower;
      xPower *= xAdjusted;
    }
  }

  VecT<double> fullPowers(numTerms);

  const uint32_t msb = uint32_t(1) << 31;
  for (int y = 0; y < height; ++y) {
    const double yAdjusted = yscale * y;

    double yPower = 1.0;
    for (int i = 0; i <= vDegree; ++i) {
      yPowers[i] = yPower;
      yPower *= yAdjusted;
    }

    for (int x = 0; x < width; ++x) {
      if (!(maskLine[x >> 5] & (msb >> (x & 31)))) {
        continue;
      }

      const double dataPoint = dataScale * imageLine[x];

      int pos = 0;
      for (int i = 0; i <= vDegree; ++i) {
        for (int j = 0; j <= hDegree; ++j, ++pos) {
          fullPowers[pos] = yPowers[i] * xPowers(j, x);
        }
      }

      double* p_AtA = AtA_data;
      for (int i = 0; i < numTerms; ++i) {
        const double iVal = fullPowers[i];
        Atb_data[i] += iVal * dataPoint;

        for (int j = 0; j < numTerms; ++j) {
          const double jVal = fullPowers[j];
          *p_AtA += iVal * jVal;
          ++p_AtA;
        }
      }
    }

    imageLine += imageStride;
    maskLine += maskStride;
  }
}  // PolynomialSurface::prepareDataForLeastSquares

void PolynomialSurface::fixSquareMatrixRankDeficiency(MatT<double>& mat) {
  assert(mat.cols() == mat.rows());

  const auto dim = static_cast<int>(mat.cols());
  for (int i = 0; i < dim; ++i) {
    mat(i, i) += 1e-5;  // Add a small value to the diagonal.
  }
}
}  // namespace imageproc
