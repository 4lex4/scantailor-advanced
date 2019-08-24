// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_IMAGEPROC_SAVGOLFILTER_H_
#define SCANTAILOR_IMAGEPROC_SAVGOLFILTER_H_

class QImage;
class QSize;

namespace imageproc {
/**
 * \brief Performs a grayscale smoothing using the Savitzky-Golay method.
 *
 * The Savitzky-Golay method is equivalent to fitting a small neighborhood
 * around each pixel to a polynomial, and then recalculating the pixel
 * value from it.  In practice, it achieves the same results without fitting
 * a polynomial for every pixel, so it performs quite well.
 *
 * \param src The source image.  It doesn't have to be grayscale, but
 *        the resulting image will be grayscale anyway.
 * \param windowSize The apperture size.  If it doesn't completely
 *        fit the image area, no filtering will take place.
 * \param horDegree The degree of a polynomial in horizontal direction.
 * \param vertDegree The degree of a polynomial in vertical direction.
 * \return The filtered grayscale image.
 *
 * \note The window size and degrees are not completely independent.
 *       The following inequality must be fulfilled:
 * \code
 *       window_width * window_height >= (horDegree + 1) * (vertDegree + 1)
 * \endcode
 * Good results for 300 dpi scans are achieved with 7x7 window and 4x4 degree.
 */
QImage savGolFilter(const QImage& src, const QSize& windowSize, int horDegree, int vertDegree);
}  // namespace imageproc
#endif
