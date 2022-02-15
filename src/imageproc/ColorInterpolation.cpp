// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "ColorInterpolation.h"

namespace imageproc {
QColor colorInterpolation(const QColor& from, const QColor& to, double dist) {
  dist = qBound(0.0, dist, 1.0);

  float r1, g1, b1, a1, r2, g2, b2, a2;
  from.getRgbF(&r1, &g1, &b1, &a1);
  to.getRgbF(&r2, &g2, &b2, &a2);

  const float r = r1 + (r2 - r1) * dist;
  const float g = g1 + (g2 - g1) * dist;
  const float b = b1 + (b2 - b1) * dist;
  const float a = a1 + (a2 - a1) * dist;
  return QColor::fromRgbF(r, g, b, a);
}
}  // namespace imageproc
