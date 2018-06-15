/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2007-2008  Joseph Artsimovich <joseph_a@mail.ru>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "Shear.h"
#include <cassert>
#include <cmath>
#include "RasterOp.h"

namespace imageproc {
void hShearFromTo(const BinaryImage& src,
                  BinaryImage& dst,
                  const double shear,
                  const double y_origin,
                  const BWColor background_color) {
  if (src.isNull() || dst.isNull()) {
    throw std::invalid_argument("Can't shear a null image");
  }
  if (src.size() != dst.size()) {
    throw std::invalid_argument("Can't shear when dst.size() != src.size()");
  }

  const int width = src.width();
  const int height = src.height();

  // shift = std::floor(0.5 + shear * (y + 0.5 - y_origin));
  double shift = 0.5 + shear * (0.5 - y_origin);
  const double shift_end = 0.5 + shear * (height - 0.5 - y_origin);
  auto shift1 = (int) std::floor(shift);

  if (shift1 == std::floor(shift_end)) {
    assert(shift1 == 0);
    dst = src;

    return;
  }

  int shift2 = shift1;
  int y1 = 0;
  int y2 = 0;
  for (;;) {
    ++y2;
    shift += shear;
    shift2 = (int) std::floor(shift);
    if ((shift1 != shift2) || (y2 == height)) {
      const int block_height = y2 - y1;
      if (std::abs(shift1) >= width) {
        // The shifted block would be completely off the image.
        const QRect fr(0, y1, width, block_height);
        dst.fill(fr, background_color);
      } else if (shift1 < 0) {
        // Shift to the left.
        const QRect dr(0, y1, width + shift1, block_height);
        const QPoint sp(-shift1, y1);
        rasterOp<RopSrc>(dst, dr, src, sp);
        const QRect fr(width + shift1, y1, -shift1, block_height);
        dst.fill(fr, background_color);
      } else if (shift1 > 0) {
        // Shift to the right.
        const QRect dr(shift1, y1, width - shift1, block_height);
        const QPoint sp(0, y1);
        rasterOp<RopSrc>(dst, dr, src, sp);
        const QRect fr(0, y1, shift1, block_height);
        dst.fill(fr, background_color);
      } else {
        // No shift, just copy.
        const QRect dr(0, y1, width, block_height);
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
                  const double x_origin,
                  const BWColor background_color) {
  if (src.isNull() || dst.isNull()) {
    throw std::invalid_argument("Can't shear a null image");
  }
  if (src.size() != dst.size()) {
    throw std::invalid_argument("Can't shear when dst.size() != src.size()");
  }

  const int width = src.width();
  const int height = src.height();

  // shift = std::floor(0.5 + shear * (x + 0.5 - x_origin));
  double shift = 0.5 + shear * (0.5 - x_origin);
  const double shift_end = 0.5 + shear * (width - 0.5 - x_origin);
  auto shift1 = (int) std::floor(shift);

  if (shift1 == std::floor(shift_end)) {
    assert(shift1 == 0);
    dst = src;

    return;
  }

  int shift2 = shift1;
  int x1 = 0;
  int x2 = 0;
  for (;;) {
    ++x2;
    shift += shear;
    shift2 = (int) std::floor(shift);
    if ((shift1 != shift2) || (x2 == width)) {
      const int block_width = x2 - x1;
      if (std::abs(shift1) >= height) {
        // The shifted block would be completely off the image.
        const QRect fr(x1, 0, block_width, height);
        dst.fill(fr, background_color);
      } else if (shift1 < 0) {
        // Shift upwards.
        const QRect dr(x1, 0, block_width, height + shift1);
        const QPoint sp(x1, -shift1);
        rasterOp<RopSrc>(dst, dr, src, sp);
        const QRect fr(x1, height + shift1, block_width, -shift1);
        dst.fill(fr, background_color);
      } else if (shift1 > 0) {
        // Shift downwards.
        const QRect dr(x1, shift1, block_width, height - shift1);
        const QPoint sp(x1, 0);
        rasterOp<RopSrc>(dst, dr, src, sp);
        const QRect fr(x1, 0, block_width, shift1);
        dst.fill(fr, background_color);
      } else {
        // No shift, just copy.
        const QRect dr(x1, 0, block_width, height);
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

BinaryImage hShear(const BinaryImage& src, const double shear, const double y_origin, const BWColor background_color) {
  BinaryImage dst(src.width(), src.height());
  hShearFromTo(src, dst, shear, y_origin, background_color);

  return dst;
}

BinaryImage vShear(const BinaryImage& src, const double shear, const double x_origin, const BWColor background_color) {
  BinaryImage dst(src.width(), src.height());
  vShearFromTo(src, dst, shear, x_origin, background_color);

  return dst;
}

void hShearInPlace(BinaryImage& image, const double shear, const double y_origin, const BWColor background_color) {
  hShearFromTo(image, image, shear, y_origin, background_color);
}

void vShearInPlace(BinaryImage& image, const double shear, const double x_origin, const BWColor background_color) {
  vShearFromTo(image, image, shear, x_origin, background_color);
}
}  // namespace imageproc