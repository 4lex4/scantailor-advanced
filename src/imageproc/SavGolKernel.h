// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_IMAGEPROC_SAVGOLKERNEL_H_
#define SCANTAILOR_IMAGEPROC_SAVGOLKERNEL_H_

#include <cstddef>
#include <vector>
#include "AlignedArray.h"

class QPoint;
class QSize;

namespace imageproc {
class SavGolKernel {
 public:
  SavGolKernel(const QSize& size, const QPoint& origin, int horDegree, int vertDegree);

  void recalcForOrigin(const QPoint& origin);

  int width() const { return m_width; }

  int height() const { return m_height; }

  const float* data() const { return m_kernel.data(); }

  float operator[](size_t idx) const { return m_kernel[idx]; }

 private:
  struct Rotation {
    double sin;
    double cos;

    Rotation(double s, double c) : sin(s), cos(c) {}
  };

  void QR();

  /**
   * A matrix of m_numDataPoints rows and m_numVars columns.
   * Stored row by row.
   */
  std::vector<double> m_equations;

  /**
   * The data points, in the same order as rows in m_equations.
   */
  std::vector<double> m_dataPoints;

  /**
   * The polynomial coefficients of size m_numVars.  Only exists to save
   * one allocation when recalculating the kernel for different data points.
   */
  std::vector<double> m_coeffs;

  /**
   * The rotations applied to m_equations as part of QR factorization.
   * Later these same rotations are applied to a copy of m_dataPoints.
   * We could avoid storing rotations and rotate m_dataPoints on the fly,
   * but in that case we would have to rotate m_equations again when
   * recalculating the kernel for different data points.
   */
  std::vector<Rotation> m_rotations;

  /**
   * 16-byte aligned convolution kernel of size m_numDataPoints.
   */
  AlignedArray<float, 4> m_kernel;

  /**
   * The degree of the polynomial in horizontal direction.
   */
  int m_horDegree;

  /**
   * The degree of the polynomial in vertical direction.
   */
  int m_vertDegree;

  /**
   * The width of the convolution kernel.
   */
  int m_width;

  /**
   * The height of the convolution kernel.
   */
  int m_height;

  /**
   * The number of terms in the polynomial.
   */
  int m_numTerms;

  /**
   * The number of data points.  This corresponds to the number of items
   * in the convolution kernel.
   */
  int m_numDataPoints;
};
}  // namespace imageproc
#endif  // ifndef SCANTAILOR_IMAGEPROC_SAVGOLKERNEL_H_
