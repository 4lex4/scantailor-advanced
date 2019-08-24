// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_IMAGEPROC_SCALE_H_
#define SCANTAILOR_IMAGEPROC_SCALE_H_

class QSize;

namespace imageproc {
class GrayImage;

/**
 * \brief Converts an image to grayscale and scales it to dstSize.
 *
 * \param src The source image.
 * \param dstSize The size to scale the image to.
 * \return The scaled image.
 *
 * This function is a faster replacement for QImage::scaled(), when
 * dealing with grayscale images.
 */
GrayImage scaleToGray(const GrayImage& src, const QSize& dstSize);
}  // namespace imageproc
#endif
