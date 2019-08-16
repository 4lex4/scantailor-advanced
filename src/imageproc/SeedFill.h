// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef IMAGEPROC_SEEDFILL_H_
#define IMAGEPROC_SEEDFILL_H_

#include "BinaryImage.h"
#include "Connectivity.h"

class QImage;

namespace imageproc {
class GrayImage;

/**
 * \brief Spread black pixels from seed as long as mask allows it.
 *
 * This operation retains black connected componets from \p mask that are
 * tagged by at least one black pixel in \p seed.  The rest do not appear
 * in the result.
 * \par
 * \p seed is allowed to contain black pixels that are not in \p mask.
 * They will be ignored and will not appear in the resulting image.
 * \par
 * The underlying code implements Luc Vincent's iterative seed-fill
 * algorithm: http://www.vincent-net.com/luc/papers/93ieeeip_recons.pdf
 */
BinaryImage seedFill(const BinaryImage& seed, const BinaryImage& mask, Connectivity connectivity);

/**
 * \brief Spread darker colors from seed as long as mask allows it.
 *
 * The result of this operation is an image where some areas are lighter
 * than in \p mask, because there were no dark paths linking them to dark
 * areas in \p seed.
 * \par
 * \p seed is allowed to contain pixels darker than the corresponding pixels
 * in \p mask.  Such pixels will be made equal to their mask values.
 * \par
 * The underlying code implements Luc Vincent's hybrid seed-fill algorithm:
 * http://www.vincent-net.com/luc/papers/93ieeeip_recons.pdf
 */
GrayImage seedFillGray(const GrayImage& seed, const GrayImage& mask, Connectivity connectivity);

/**
 * \brief A faster, in-place version of seedFillGray().
 */
void seedFillGrayInPlace(GrayImage& seed, const GrayImage& mask, Connectivity connectivity);

/**
 * \brief A slower but more simple implementation of seedFillGray().
 *
 * This function should not be used for anything but testing the correctness
 * of the fast and complex implementation that is seedFillGray().
 */
GrayImage seedFillGraySlow(const GrayImage& seed, const GrayImage& mask, Connectivity connectivity);
}  // namespace imageproc
#endif
