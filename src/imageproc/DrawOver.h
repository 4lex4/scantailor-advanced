// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_IMAGEPROC_DRAWOVER_H_
#define SCANTAILOR_IMAGEPROC_DRAWOVER_H_

class QImage;
class QRect;

namespace imageproc {
/**
 * \brief Overdraws a portion of one image with a portion of another.
 *
 * \param dst The destination image.  Can be in any format, as long
 *        as the source image has the same format.
 * \param dstRect The area of the destination image to be overdrawn.
 *        This area must lie completely within the destination
 *        image, and its size must match the size of \p srcRect.
 * \param src The source image.  Can be in any format, as long
 *        as the destination image has the same format.
 * \param srcRect The area of the source image to draw over
 *        the destination image.  This area must lie completely
 *        within the source image, and its size must match the
 *        size of \p dstRect.
 */
void drawOver(QImage& dst, const QRect& dstRect, const QImage& src, const QRect& srcRect);
}  // namespace imageproc
#endif
