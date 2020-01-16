// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "Shear.h"

#include <cassert>
#include <cmath>

#include "RasterOp.h"

namespace imageproc {
void hShearFromTo(const BinaryImage& src,
                  BinaryImage& dst,
                  const double shear,
                  const double yOrigin,
                  const BWColor backgroundColor) {
  if (src.isNull() || dst.isNull()) {
    throw std::invalid_argument("Can't shear a null image");
  }
  if (src.size() != dst.size()) {
    throw std::invalid_argument("Can't shear when dst.size() != src.size()");
  }

  const int width = src.width();
  const int height = src.height();

  // shift = std::floor(0.5 + shear * (y + 0.5 - yOrigin));
  double shift = 0.5 + shear * (0.5 - yOrigin);
  const double shiftEnd = 0.5 + shear * (height - 0.5 - yOrigin);
  auto shift1 = (int) std::floor(shift);

  if (shift1 == std::floor(shiftEnd)) {
    assert(shift1 == 0);
    dst = src;
    return;
  }

  int shift2 = shift1;
  int y1 = 0;
  int y2 = 0;
  while (true) {
    ++y2;
    shift += shear;
    shift2 = (int) std::floor(shift);
    if ((shift1 != shift2) || (y2 == height)) {
      const int blockHeight = y2 - y1;
      if (std::abs(shift1) >= width) {
        // The shifted block would be completely off the image.
        const QRect fr(0, y1, width, blockHeight);
        dst.fill(fr, backgroundColor);
      } else if (shift1 < 0) {
        // Shift to the left.
        const QRect dr(0, y1, width + shift1, blockHeight);
        const QPoint sp(-shift1, y1);
        rasterOp<RopSrc>(dst, dr, src, sp);
        const QRect fr(width + shift1, y1, -shift1, blockHeight);
        dst.fill(fr, backgroundColor);
      } else if (shift1 > 0) {
        // Shift to the right.
        const QRect dr(shift1, y1, width - shift1, blockHeight);
        const QPoint sp(0, y1);
        rasterOp<RopSrc>(dst, dr, src, sp);
        const QRect fr(0, y1, shift1, blockHeight);
        dst.fill(fr, backgroundColor);
      } else {
        // No shift, just copy.
        const QRect dr(0, y1, width, blockHeight);
        const QPoint sp(0, y1);
        rasterOp<RopSrc>(dst, dr, src, sp);
      }

      if (y2 == height) {
        break;
      }

      y1 = y2;
      shift1 = shift2;
    }
  }
}  // hShearFromTo

void vShearFromTo(const BinaryImage& src,
                  BinaryImage& dst,
                  const double shear,
                  const double xOrigin,
                  const BWColor backgroundColor) {
  if (src.isNull() || dst.isNull()) {
    throw std::invalid_argument("Can't shear a null image");
  }
  if (src.size() != dst.size()) {
    throw std::invalid_argument("Can't shear when dst.size() != src.size()");
  }

  const int width = src.width();
  const int height = src.height();

  // shift = std::floor(0.5 + shear * (x + 0.5 - xOrigin));
  double shift = 0.5 + shear * (0.5 - xOrigin);
  const double shiftEnd = 0.5 + shear * (width - 0.5 - xOrigin);
  auto shift1 = (int) std::floor(shift);

  if (shift1 == std::floor(shiftEnd)) {
    assert(shift1 == 0);
    dst = src;
    return;
  }

  int shift2 = shift1;
  int x1 = 0;
  int x2 = 0;
  while (true) {
    ++x2;
    shift += shear;
    shift2 = (int) std::floor(shift);
    if ((shift1 != shift2) || (x2 == width)) {
      const int blockWidth = x2 - x1;
      if (std::abs(shift1) >= height) {
        // The shifted block would be completely off the image.
        const QRect fr(x1, 0, blockWidth, height);
        dst.fill(fr, backgroundColor);
      } else if (shift1 < 0) {
        // Shift upwards.
        const QRect dr(x1, 0, blockWidth, height + shift1);
        const QPoint sp(x1, -shift1);
        rasterOp<RopSrc>(dst, dr, src, sp);
        const QRect fr(x1, height + shift1, blockWidth, -shift1);
        dst.fill(fr, backgroundColor);
      } else if (shift1 > 0) {
        // Shift downwards.
        const QRect dr(x1, shift1, blockWidth, height - shift1);
        const QPoint sp(x1, 0);
        rasterOp<RopSrc>(dst, dr, src, sp);
        const QRect fr(x1, 0, blockWidth, shift1);
        dst.fill(fr, backgroundColor);
      } else {
        // No shift, just copy.
        const QRect dr(x1, 0, blockWidth, height);
        const QPoint sp(x1, 0);
        rasterOp<RopSrc>(dst, dr, src, sp);
      }

      if (x2 == width) {
        break;
      }

      x1 = x2;
      shift1 = shift2;
    }
  }
}  // vShearFromTo

BinaryImage hShear(const BinaryImage& src, const double shear, const double yOrigin, const BWColor backgroundColor) {
  BinaryImage dst(src.width(), src.height());
  hShearFromTo(src, dst, shear, yOrigin, backgroundColor);
  return dst;
}

BinaryImage vShear(const BinaryImage& src, const double shear, const double xOrigin, const BWColor backgroundColor) {
  BinaryImage dst(src.width(), src.height());
  vShearFromTo(src, dst, shear, xOrigin, backgroundColor);
  return dst;
}

void hShearInPlace(BinaryImage& image, const double shear, const double yOrigin, const BWColor backgroundColor) {
  hShearFromTo(image, image, shear, yOrigin, backgroundColor);
}

void vShearInPlace(BinaryImage& image, const double shear, const double xOrigin, const BWColor backgroundColor) {
  vShearFromTo(image, image, shear, xOrigin, backgroundColor);
}
}  // namespace imageproc