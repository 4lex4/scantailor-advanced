// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.


#ifndef SCANTAILOR_IMAGEPROC_SHEAR_H_
#define SCANTAILOR_IMAGEPROC_SHEAR_H_

#include "BWColor.h"

namespace imageproc {
class BinaryImage;

/**
 * \brief Horizontal shear.
 *
 * \param src The source image.
 * \param dst The desctination image.
 * \param shear The shift of each next line relative to the previous one.
 * \param yOrigin The y value where a line would have a zero shift.
 *        Note that a value of 1.0 doesn't mean that line 1
 *        is to have zero shift.  The value of 1.5 would mean that.
 * \param backgroundColor The color used to fill areas not represented
 *        in the source image.
 * \note The source and destination images must have the same size.
 */
void hShearFromTo(const BinaryImage& src, BinaryImage& dst, double shear, double yOrigin, BWColor backgroundColor);

/**
 * \brief Vertical shear.
 *
 * \param src The source image.
 * \param dst The destination image.
 * \param shear The shift of each next line relative to the previous one.
 * \param xOrigin The x value where a line would have a zero shift.
 *        Note that a value of 1.0 doesn't mean that line 1
 *        is to have zero shift.  The value of 1.5 would mean that.
 * \param backgroundColor The color used to fill areas not represented
 *        in the source image.
 * \note The source and destination images must have the same size.
 */
void vShearFromTo(const BinaryImage& src, BinaryImage& dst, double shear, double xOrigin, BWColor backgroundColor);

/**
 * \brief Horizontal shear returing a new image.
 *
 * Same as hShearFromTo(), but creates and returns the destination image.
 */
BinaryImage hShear(const BinaryImage& src, double shear, double yOrigin, BWColor backgroundColor);

/**
 * \brief Vertical shear returning a new image.
 *
 * Same as vShearFromTo(), but creates and returns the destination image.
 */
BinaryImage vShear(const BinaryImage& src, double shear, double xOrigin, BWColor backgroundColor);

/**
 * \brief In-place horizontal shear.
 *
 * Same as hShearFromTo() with src and dst being the same image.
 */
void hShearInPlace(BinaryImage& image, double shear, double yOrigin, BWColor backgroundColor);

/**
 * \brief In-place vertical shear.
 *
 * Same as vShearFromTo() with src and dst being the same image.
 */
void vShearInPlace(BinaryImage& image, double shear, double xOrigin, BWColor backgroundColor);
}  // namespace imageproc
#endif
