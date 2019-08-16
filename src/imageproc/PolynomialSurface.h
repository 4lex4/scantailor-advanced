// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef IMAGEPROC_POLYNOMIAL_SURFACE_H_
#define IMAGEPROC_POLYNOMIAL_SURFACE_H_

#include <QSize>
#include <cstdint>
#include "MatT.h"
#include "VecT.h"

namespace imageproc {
class BinaryImage;
class GrayImage;

/**
 * \brief A polynomial function describing a 2D surface.
 */
class PolynomialSurface {
  // Member-wise copying is OK.
 public:
  /**
   * \brief Calculate a polynomial that approximates the given image.
   *
   * \param hor_degree The degree of the polynomial in horizontal direction.
   *        Must not be negative.  A value of 3 or 4 should be enough
   *        to approximate page background.
   * \param vert_degree The degree of the polynomial in vertical direction.
   *        Must not be negative.  A value of 3 or 4 should be enough
   *        to approximate page background.
   * \param src The image to approximate.  Must be grayscale and not null.
   *
   * \note Building a polynomial surface for full size 300 DPI scans
   *       takes forever, so pass a downscaled version here. 300x300
   *       pixels will be fine.  Once built, the polynomial surface
   *       may then be rendered in the original size, if necessary.
   */
  PolynomialSurface(int hor_degree, int vert_degree, const GrayImage& src);

  /**
   * \brief Calculate a polynomial that approximates portions of the given image.
   *
   * \param hor_degree The degree of the polynomial in horizontal direction.
   *        Must not be negative.  A value of 5 should be enough
   *        to approximate page background.
   * \param vert_degree The degree of the polynomial in vertical direction.
   *        Must not be negative.  A value of 5 should be enough
   *        to approximate page background.
   * \param src The image to approximate.  Must be grayscale and not null.
   * \param mask Specifies which areas of \p src to consider.
   *        A pixel in \p src is considered if the corresponding pixel
   *        in \p mask is black.
   *
   * \note Building a polynomial surface for full size 300 DPI scans
   *       takes forever, so pass a downscaled version here. 300x300
   *       pixels will be fine.  Once built, the polynomial surface
   *       may then rendered in the original size, if necessary.
   */
  PolynomialSurface(int hor_degree, int vert_degree, const GrayImage& src, const BinaryImage& mask);

  /**
   * \brief Visualizes the polynomial surface as a grayscale image.
   *
   * The surface will be stretched / shrunk to fit the new size.
   */
  GrayImage render(const QSize& size) const;

 private:
  void maybeReduceDegrees(int num_data_points);

  int calcNumTerms() const;

  static double calcScale(int dimension);

  static void prepareDataForLeastSquares(const GrayImage& image,
                                         MatT<double>& AtA,
                                         VecT<double>& Atb,
                                         int h_degree,
                                         int v_degree);

  static void prepareDataForLeastSquares(const GrayImage& image,
                                         const BinaryImage& mask,
                                         MatT<double>& AtA,
                                         VecT<double>& Atb,
                                         int h_degree,
                                         int v_degree);

  static void fixSquareMatrixRankDeficiency(MatT<double>& mat);

  VecT<double> m_coeffs;
  int m_horDegree;
  int m_vertDegree;
};
}  // namespace imageproc

#endif  // ifndef IMAGEPROC_POLYNOMIAL_SURFACE_H_
