/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C)  Joseph Artsimovich <joseph.artsimovich@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>
 */

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
PolynomialSurface::PolynomialSurface(const int hor_degree, const int vert_degree, const GrayImage& src)
    : m_horDegree(hor_degree), m_vertDegree(vert_degree) {
  // Note: m_horDegree and m_vertDegree may still change!

  if (hor_degree < 0) {
    throw std::invalid_argument("PolynomialSurface: horizontal degree is invalid");
  }
  if (vert_degree < 0) {
    throw std::invalid_argument("PolynomialSurface: vertical degree is invalid");
  }

  const int num_data_points = src.width() * src.height();
  if (num_data_points == 0) {
    m_horDegree = 0;
    m_vertDegree = 0;
    VecT<double>(1, 0.0).swap(m_coeffs);
    return;
  }

  maybeReduceDegrees(num_data_points);

  const int num_terms = calcNumTerms();
  VecT<double>(num_terms, 0.0).swap(m_coeffs);

  // The least squares equation is A^T*A*x = A^T*b
  // We will be building A^T*A and A^T*b incrementally.
  // This allows us not to build matrix A at all.
  MatT<double> AtA(num_terms, num_terms);
  VecT<double> Atb(num_terms);
  prepareDataForLeastSquares(src, AtA, Atb, m_horDegree, m_vertDegree);

  fixSquareMatrixRankDeficiency(AtA);

  try {
    DynamicMatrixCalc<double> mc;
    mc(AtA).solve(mc(Atb)).write(m_coeffs.data());
  } catch (const std::runtime_error&) {
  }
}

PolynomialSurface::PolynomialSurface(const int hor_degree,
                                     const int vert_degree,
                                     const GrayImage& src,
                                     const BinaryImage& mask)
    : m_horDegree(hor_degree), m_vertDegree(vert_degree) {
  // Note: m_horDegree and m_vertDegree may still change!

  if (hor_degree < 0) {
    throw std::invalid_argument("PolynomialSurface: horizontal degree is invalid");
  }
  if (vert_degree < 0) {
    throw std::invalid_argument("PolynomialSurface: vertical degree is invalid");
  }
  if (src.size() != mask.size()) {
    throw std::invalid_argument("PolynomialSurface: image and mask have different sizes");
  }

  const int num_data_points = mask.countBlackPixels();
  if (num_data_points == 0) {
    m_horDegree = 0;
    m_vertDegree = 0;
    VecT<double>(1, 0.0).swap(m_coeffs);
    return;
  }

  maybeReduceDegrees(num_data_points);

  const int num_terms = calcNumTerms();
  VecT<double>(num_terms, 0.0).swap(m_coeffs);

  // The least squares equation is A^T*A*x = A^T*b
  // We will be building A^T*A and A^T*b incrementally.
  // This allows us not to build matrix A at all.
  MatT<double> AtA(num_terms, num_terms);
  VecT<double> Atb(num_terms);
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
  const auto num_coeffs = static_cast<const int>(m_coeffs.size());

  // Pretend that both x and y positions of pixels
  // lie in range of [0, 1].
  const double xscale = calcScale(width);
  const double yscale = calcScale(height);

  AlignedArray<float, 4> vert_matrix(num_coeffs * height);
  float* out = &vert_matrix[0];
  for (int y = 0; y < height; ++y) {
    const double y_adjusted = y * yscale;
    double pow = 1.0;
    int pos = 0;
    for (int i = 0; i <= m_vertDegree; ++i) {
      for (int j = 0; j <= m_horDegree; ++j, ++pos, ++out) {
        *out = static_cast<float>(m_coeffs[pos] * pow);
      }
      pow *= y_adjusted;
    }
  }

  AlignedArray<float, 4> hor_matrix(num_coeffs * width);
  out = &hor_matrix[0];
  for (int x = 0; x < width; ++x) {
    const double x_adjusted = x * xscale;
    for (int i = 0; i <= m_vertDegree; ++i) {
      double pow = 1.0;
      for (int j = 0; j <= m_horDegree; ++j, ++out) {
        *out = static_cast<float>(pow);
        pow *= x_adjusted;
      }
    }
  }

  const float* vert_line = &vert_matrix[0];
  for (int y = 0; y < height; ++y, line += bpl, vert_line += num_coeffs) {
    const float* hor_line = &hor_matrix[0];
    for (int x = 0; x < width; ++x, hor_line += num_coeffs) {
      float sum = 0.5f / 255.0f;  // for rounding purposes.
      for (int i = 0; i < num_coeffs; ++i) {
        sum += hor_line[i] * vert_line[i];
      }
      const auto isum = (int) (sum * 255.0);
      line[x] = static_cast<unsigned char>(qBound(0, isum, 255));
    }
  }

  return image;
}  // PolynomialSurface::render

void PolynomialSurface::maybeReduceDegrees(const int num_data_points) {
  assert(num_data_points > 0);

  while (num_data_points < calcNumTerms()) {
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
                                                   const int h_degree,
                                                   const int v_degree) {
  double* const AtA_data = AtA.data();
  double* const Atb_data = Atb.data();

  const int width = image.width();
  const int height = image.height();
  const auto num_terms = static_cast<const int>(Atb.size());

  const uint8_t* line = image.data();
  const int stride = image.stride();

  // Pretend that both x and y positions of pixels
  // lie in range of [0, 1].
  const double xscale = calcScale(width);
  const double yscale = calcScale(height);

  // To force data samples into [0, 1] range.
  const double data_scale = 1.0 / 255.0;

  // 1, y, y^2, y^3, ...
  VecT<double> y_powers(v_degree + 1);  // Initialized to 0.

  // Same as y_powers, except y_powers correspond to a given y,
  // while x_powers are computed for all possible x values.
  MatT<double> x_powers(h_degree + 1, width);  // Initialized to 0.
  for (int x = 0; x < width; ++x) {
    const double x_adjusted = xscale * x;
    double x_power = 1.0;
    for (int i = 0; i <= h_degree; ++i) {
      x_powers(i, x) = x_power;
      x_power *= x_adjusted;
    }
  }

  VecT<double> full_powers(num_terms);

  for (int y = 0; y < height; ++y, line += stride) {
    const double y_adjusted = yscale * y;

    double y_power = 1.0;
    for (int i = 0; i <= v_degree; ++i) {
      y_powers[i] = y_power;
      y_power *= y_adjusted;
    }

    for (int x = 0; x < width; ++x) {
      const double data_point = data_scale * line[x];

      int pos = 0;
      for (int i = 0; i <= v_degree; ++i) {
        for (int j = 0; j <= h_degree; ++j, ++pos) {
          full_powers[pos] = y_powers[i] * x_powers(j, x);
        }
      }

      double* p_AtA = AtA_data;
      for (int i = 0; i < num_terms; ++i) {
        const double i_val = full_powers[i];
        Atb_data[i] += i_val * data_point;

        for (int j = 0; j < num_terms; ++j) {
          const double j_val = full_powers[j];
          *p_AtA += i_val * j_val;
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
                                                   const int h_degree,
                                                   const int v_degree) {
  double* const AtA_data = AtA.data();
  double* const Atb_data = Atb.data();

  const int width = image.width();
  const int height = image.height();
  const auto num_terms = static_cast<const int>(Atb.size());

  const uint8_t* image_line = image.data();
  const int image_stride = image.stride();

  const uint32_t* mask_line = mask.data();
  const int mask_stride = mask.wordsPerLine();

  // Pretend that both x and y positions of pixels
  // lie in range of [0, 1].
  const double xscale = calcScale(width);
  const double yscale = calcScale(height);

  // To force data samples into [0, 1] range.
  const double data_scale = 1.0 / 255.0;

  // 1, y, y^2, y^3, ...
  VecT<double> y_powers(v_degree + 1);  // Initialized to 0.

  // Same as y_powers, except y_powers correspond to a given y,
  // while x_powers are computed for all possible x values.
  MatT<double> x_powers(h_degree + 1, width);  // Initialized to 0.
  for (int x = 0; x < width; ++x) {
    const double x_adjusted = xscale * x;
    double x_power = 1.0;
    for (int i = 0; i <= h_degree; ++i) {
      x_powers(i, x) = x_power;
      x_power *= x_adjusted;
    }
  }

  VecT<double> full_powers(num_terms);

  const uint32_t msb = uint32_t(1) << 31;
  for (int y = 0; y < height; ++y) {
    const double y_adjusted = yscale * y;

    double y_power = 1.0;
    for (int i = 0; i <= v_degree; ++i) {
      y_powers[i] = y_power;
      y_power *= y_adjusted;
    }

    for (int x = 0; x < width; ++x) {
      if (!(mask_line[x >> 5] & (msb >> (x & 31)))) {
        continue;
      }

      const double data_point = data_scale * image_line[x];

      int pos = 0;
      for (int i = 0; i <= v_degree; ++i) {
        for (int j = 0; j <= h_degree; ++j, ++pos) {
          full_powers[pos] = y_powers[i] * x_powers(j, x);
        }
      }

      double* p_AtA = AtA_data;
      for (int i = 0; i < num_terms; ++i) {
        const double i_val = full_powers[i];
        Atb_data[i] += i_val * data_point;

        for (int j = 0; j < num_terms; ++j) {
          const double j_val = full_powers[j];
          *p_AtA += i_val * j_val;
          ++p_AtA;
        }
      }
    }

    image_line += image_stride;
    mask_line += mask_stride;
  }
}  // PolynomialSurface::prepareDataForLeastSquares

void PolynomialSurface::fixSquareMatrixRankDeficiency(MatT<double>& mat) {
  assert(mat.cols() == mat.rows());

  const auto dim = static_cast<const int>(mat.cols());
  for (int i = 0; i < dim; ++i) {
    mat(i, i) += 1e-5;  // Add a small value to the diagonal.
  }
}
}  // namespace imageproc
