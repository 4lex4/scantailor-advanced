// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_IMAGEPROC_UPSCALEINTEGERTIMES_H_
#define SCANTAILOR_IMAGEPROC_UPSCALEINTEGERTIMES_H_

#include "BWColor.h"

class QSize;

namespace imageproc {
class BinaryImage;

/**
 * \brief Upscale a binary image integer times in each direction.
 */
BinaryImage upscaleIntegerTimes(const BinaryImage& src, int xscale, int yscale);

/**
 * \brief Upscale a binary image integer times in each direction
 *        and add padding if necessary.
 *
 * The resulting image will have a size of \p dstSize, which is achieved
 * by upscaling the source image integer times in each direction and then
 * adding a padding to reach the requested size.
 */
BinaryImage upscaleIntegerTimes(const BinaryImage& src, const QSize& dstSize, BWColor padding);
}  // namespace imageproc
#endif
