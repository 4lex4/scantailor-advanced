// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef IMAGEPROC_COLOR_FOR_ID_H_
#define IMAGEPROC_COLOR_FOR_ID_H_

#include <QColor>
#include "BitOps.h"

namespace imageproc {
/**
 * \brief Generates a color corresponding to a particular numeric ID.
 *
 * Colors for IDs that are numerically close will tend to be significantly
 * different.  Positive IDs are handled better.
 */
template <typename T>
QColor colorForId(T id) {
  const int bits_unused = countMostSignificantZeroes(id);
  const int bits_used = sizeof(T) * 8 - bits_unused;
  const T reversed = reverseBits(id) >> bits_unused;
  const T mask = (T(1) << bits_used) - 1;

  const double H = 0.99 * double(reversed + 1) / (mask + 1);
  const double S = 1.0;
  const double V = 1.0;
  QColor color;
  color.setHsvF(H, S, V);

  return color;
}
}  // namespace imageproc
#endif
