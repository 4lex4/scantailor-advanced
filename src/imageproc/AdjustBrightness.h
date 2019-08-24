// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_IMAGEPROC_ADJUSTBRIGHTNESS_H_
#define SCANTAILOR_IMAGEPROC_ADJUSTBRIGHTNESS_H_

class QImage;

namespace imageproc {
/**
 * \brief Recalculates every pixel to make its brightness match the provided level.
 *
 * \param rgbImage The image to adjust brightness in.  Must be
 *        QImage::Format_RGB32 or QImage::Format_ARGB32.
 *        The alpha channel won't be modified.
 * \param brightness A grayscale image representing the desired brightness
 *        of each pixel.  Must be QImage::Format_Indexed8, have a grayscale
 *        palette and have the same size as \p rgbImage.
 * \param wr The weighting factor for red color component in the brightness
 *        image.
 * \param wb The weighting factor for blue color component in the brightness
 *        image.
 *
 * The brightness values are normally a weighted sum of red, green and blue
 * color components.  The formula is:
 * \code
 * brightness = R * wr + G * wg + B * wb;
 * \endcode
 * This function takes wr and wb arguments, and calculates wg as 1.0 - wr - wb.
 */
void adjustBrightness(QImage& rgbImage, const QImage& brightness, double wr, double wb);

/**
 * \brief A custom version of adjustBrightness().
 *
 * Same as adjustBrightness(), but the weighting factors used in the YUV
 * color space are assumed.
 */
void adjustBrightnessYUV(QImage& rgbImage, const QImage& brightness);

/**
 * \brief A custom version of adjustBrightness().
 *
 * Same as adjustBrightness(), but the weighting factors used by
 * toGrayscale() and qGray() are assumed.
 */
void adjustBrightnessGrayscale(QImage& rgbImage, const QImage& brightness);
}  // namespace imageproc
#endif
