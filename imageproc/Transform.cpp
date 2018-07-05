/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C)  Joseph Artsimovich <joseph.artsimovich@gmail.com>

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

#include "Transform.h"
#include <QDebug>
#include <cassert>
#include "BadAllocIfNull.h"
#include "ColorMixer.h"
#include "Grayscale.h"

namespace imageproc {
namespace {
struct XLess {
  bool operator()(const QPointF& lhs, const QPointF& rhs) const { return lhs.x() < rhs.x(); }
};

struct YLess {
  bool operator()(const QPointF& lhs, const QPointF& rhs) const { return lhs.y() < rhs.y(); }
};

QSizeF calcSrcUnitSize(const QTransform& xform, const QSizeF& min) {
  // Imagine a rectangle of (0, 0, 1, 1), except we take
  // centers of its edges instead of its vertices.
  QPolygonF dst_poly;
  dst_poly.push_back(QPointF(0.5, 0.0));
  dst_poly.push_back(QPointF(1.0, 0.5));
  dst_poly.push_back(QPointF(0.5, 1.0));
  dst_poly.push_back(QPointF(0.0, 0.5));

  QPolygonF src_poly(xform.map(dst_poly));
  std::sort(src_poly.begin(), src_poly.end(), XLess());
  const double width = src_poly.back().x() - src_poly.front().x();
  std::sort(src_poly.begin(), src_poly.end(), YLess());
  const double height = src_poly.back().y() - src_poly.front().y();

  const QSizeF min32(min * 32.0);

  return QSizeF(std::max(min32.width(), width), std::max(min32.height(), height));
}

template <typename StorageUnit, typename Mixer>
static void transformGeneric(const StorageUnit* const src_data,
                             const int src_stride,
                             const QSize src_size,
                             StorageUnit* const dst_data,
                             const int dst_stride,
                             const QTransform& xform,
                             const QRect& dst_rect,
                             const StorageUnit outside_color,
                             const int outside_flags,
                             const QSizeF& min_mapping_area) {
  const int sw = src_size.width();
  const int sh = src_size.height();
  const int dw = dst_rect.width();
  const int dh = dst_rect.height();

  StorageUnit* dst_line = dst_data;

  QTransform inv_xform;
  inv_xform.translate(dst_rect.x(), dst_rect.y());
  inv_xform *= xform.inverted();
  inv_xform *= QTransform().scale(32.0, 32.0);

  // sx32 = dx*inv_xform.m11() + dy*inv_xform.m21() + inv_xform.dx();
  // sy32 = dy*inv_xform.m22() + dx*inv_xform.m12() + inv_xform.dy();

  const QSizeF src32_unit_size(calcSrcUnitSize(inv_xform, min_mapping_area));
  const int src32_unit_w = std::max<int>(1, qRound(src32_unit_size.width()));
  const int src32_unit_h = std::max<int>(1, qRound(src32_unit_size.height()));

  for (int dy = 0; dy < dh; ++dy, dst_line += dst_stride) {
    const double f_dy_center = dy + 0.5;
    const double f_sx32_base = f_dy_center * inv_xform.m21() + inv_xform.dx();
    const double f_sy32_base = f_dy_center * inv_xform.m22() + inv_xform.dy();

    for (int dx = 0; dx < dw; ++dx) {
      const double f_dx_center = dx + 0.5;
      const double f_sx32_center = f_sx32_base + f_dx_center * inv_xform.m11();
      const double f_sy32_center = f_sy32_base + f_dx_center * inv_xform.m12();
      int src32_left = (int) f_sx32_center - (src32_unit_w >> 1);
      int src32_top = (int) f_sy32_center - (src32_unit_h >> 1);
      int src32_right = src32_left + src32_unit_w;
      int src32_bottom = src32_top + src32_unit_h;
      int src_left = src32_left >> 5;
      int src_right = (src32_right - 1) >> 5;  // inclusive
      int src_top = src32_top >> 5;
      int src_bottom = (src32_bottom - 1) >> 5;  // inclusive
      assert(src_bottom >= src_top);
      assert(src_right >= src_left);

      if ((src_bottom < 0) || (src_right < 0) || (src_left >= sw) || (src_top >= sh)) {
        // Completely outside of src image.
        if (outside_flags & OutsidePixels::COLOR) {
          dst_line[dx] = outside_color;
        } else {
          const int src_x = qBound<int>(0, (src_left + src_right) >> 1, sw - 1);
          const int src_y = qBound<int>(0, (src_top + src_bottom) >> 1, sh - 1);
          dst_line[dx] = src_data[src_y * src_stride + src_x];
        }
        continue;
      }

      /*
       * Note that (intval / 32) is not the same as (intval >> 5).
       * The former rounds towards zero, while the latter rounds towards
       * negative infinity.
       * Likewise, (intval % 32) is not the same as (intval & 31).
       * The following expression:
       * top_fraction = 32 - (src32_top & 31);
       * works correctly with both positive and negative src32_top.
       */

      unsigned background_area = 0;

      if (src_top < 0) {
        const unsigned top_fraction = 32 - (src32_top & 31);
        const unsigned hor_fraction = src32_right - src32_left;
        background_area += top_fraction * hor_fraction;
        const unsigned full_pixels_ver = -1 - src_top;
        background_area += hor_fraction * (full_pixels_ver << 5);
        src_top = 0;
        src32_top = 0;
      }
      if (src_bottom >= sh) {
        const unsigned bottom_fraction = src32_bottom - (src_bottom << 5);
        const unsigned hor_fraction = src32_right - src32_left;
        background_area += bottom_fraction * hor_fraction;
        const unsigned full_pixels_ver = src_bottom - sh;
        background_area += hor_fraction * (full_pixels_ver << 5);
        src_bottom = sh - 1;     // inclusive
        src32_bottom = sh << 5;  // exclusive
      }
      if (src_left < 0) {
        const unsigned left_fraction = 32 - (src32_left & 31);
        const unsigned vert_fraction = src32_bottom - src32_top;
        background_area += left_fraction * vert_fraction;
        const unsigned full_pixels_hor = -1 - src_left;
        background_area += vert_fraction * (full_pixels_hor << 5);
        src_left = 0;
        src32_left = 0;
      }
      if (src_right >= sw) {
        const unsigned right_fraction = src32_right - (src_right << 5);
        const unsigned vert_fraction = src32_bottom - src32_top;
        background_area += right_fraction * vert_fraction;
        const unsigned full_pixels_hor = src_right - sw;
        background_area += vert_fraction * (full_pixels_hor << 5);
        src_right = sw - 1;     // inclusive
        src32_right = sw << 5;  // exclusive
      }
      assert(src_bottom >= src_top);
      assert(src_right >= src_left);

      Mixer mixer;
      if (outside_flags & OutsidePixels::WEAK) {
        background_area = 0;
      } else {
        assert(outside_flags & OutsidePixels::COLOR);
        mixer.add(outside_color, background_area);
      }

      const unsigned left_fraction = 32 - (src32_left & 31);
      const unsigned top_fraction = 32 - (src32_top & 31);
      const unsigned right_fraction = src32_right - (src_right << 5);
      const unsigned bottom_fraction = src32_bottom - (src_bottom << 5);

      assert(left_fraction + right_fraction + (src_right - src_left - 1) * 32
             == static_cast<unsigned>(src32_right - src32_left));
      assert(top_fraction + bottom_fraction + (src_bottom - src_top - 1) * 32
             == static_cast<unsigned>(src32_bottom - src32_top));

      const unsigned src_area = (src32_bottom - src32_top) * (src32_right - src32_left);
      if (src_area == 0) {
        if ((outside_flags & OutsidePixels::COLOR)) {
          dst_line[dx] = outside_color;
        } else {
          const int src_x = qBound<int>(0, (src_left + src_right) >> 1, sw - 1);
          const int src_y = qBound<int>(0, (src_top + src_bottom) >> 1, sh - 1);
          dst_line[dx] = src_data[src_y * src_stride + src_x];
        }
        continue;
      }

      const StorageUnit* src_line = &src_data[src_top * src_stride];

      if (src_top == src_bottom) {
        if (src_left == src_right) {
          // dst pixel maps to a single src pixel
          const StorageUnit c = src_line[src_left];
          if (background_area == 0) {
            // common case optimization
            dst_line[dx] = c;
            continue;
          }
          mixer.add(c, src_area);
        } else {
          // dst pixel maps to a horizontal line of src pixels
          const unsigned vert_fraction = src32_bottom - src32_top;
          const unsigned left_area = vert_fraction * left_fraction;
          const unsigned middle_area = vert_fraction << 5;
          const unsigned right_area = vert_fraction * right_fraction;

          mixer.add(src_line[src_left], left_area);

          for (int sx = src_left + 1; sx < src_right; ++sx) {
            mixer.add(src_line[sx], middle_area);
          }

          mixer.add(src_line[src_right], right_area);
        }
      } else if (src_left == src_right) {
        // dst pixel maps to a vertical line of src pixels
        const unsigned hor_fraction = src32_right - src32_left;
        const unsigned top_area = hor_fraction * top_fraction;
        const unsigned middle_area = hor_fraction << 5;
        const unsigned bottom_area = hor_fraction * bottom_fraction;

        src_line += src_left;
        mixer.add(*src_line, top_area);

        src_line += src_stride;

        for (int sy = src_top + 1; sy < src_bottom; ++sy) {
          mixer.add(*src_line, middle_area);
          src_line += src_stride;
        }

        mixer.add(*src_line, bottom_area);
      } else {
        // dst pixel maps to a block of src pixels
        const unsigned top_area = top_fraction << 5;
        const unsigned bottom_area = bottom_fraction << 5;
        const unsigned left_area = left_fraction << 5;
        const unsigned right_area = right_fraction << 5;
        const unsigned topleft_area = top_fraction * left_fraction;
        const unsigned topright_area = top_fraction * right_fraction;
        const unsigned bottomleft_area = bottom_fraction * left_fraction;
        const unsigned bottomright_area = bottom_fraction * right_fraction;

        // process the top-left corner
        mixer.add(src_line[src_left], topleft_area);

        // process the top line (without corners)
        for (int sx = src_left + 1; sx < src_right; ++sx) {
          mixer.add(src_line[sx], top_area);
        }

        // process the top-right corner
        mixer.add(src_line[src_right], topright_area);

        src_line += src_stride;
        // process middle lines
        for (int sy = src_top + 1; sy < src_bottom; ++sy) {
          mixer.add(src_line[src_left], left_area);

          for (int sx = src_left + 1; sx < src_right; ++sx) {
            mixer.add(src_line[sx], 32 * 32);
          }

          mixer.add(src_line[src_right], right_area);

          src_line += src_stride;
        }

        // process bottom-left corner
        mixer.add(src_line[src_left], bottomleft_area);

        // process the bottom line (without corners)
        for (int sx = src_left + 1; sx < src_right; ++sx) {
          mixer.add(src_line[sx], bottom_area);
        }

        // process the bottom-right corner
        mixer.add(src_line[src_right], bottomright_area);
      }

      dst_line[dx] = mixer.mix(src_area + background_area);
    }
  }
}  // transformGeneric

void fixDpiInPlace(QImage& image, const QTransform& xform) {
  if (xform.isScaling()) {
    QRect dpi_rect(QPoint(0, 0), QSize(image.dotsPerMeterX(), image.dotsPerMeterY()));
    xform.mapRect(dpi_rect);
    image.setDotsPerMeterX(dpi_rect.width());
    image.setDotsPerMeterX(dpi_rect.width());
  }
}

void fixDpiInPlace(GrayImage& image, const QTransform& xform) {
  if (xform.isScaling()) {
    QRect dpi_rect(QPoint(0, 0), QSize(image.dotsPerMeterX(), image.dotsPerMeterY()));
    xform.mapRect(dpi_rect);
    image.setDotsPerMeterX(dpi_rect.width());
    image.setDotsPerMeterX(dpi_rect.width());
  }
}
}  // namespace

QImage transform(const QImage& src,
                 const QTransform& xform,
                 const QRect& dst_rect,
                 const OutsidePixels outside_pixels,
                 const QSizeF& min_mapping_area) {
  if (src.isNull() || dst_rect.isEmpty()) {
    return QImage();
  }

  if (!xform.isAffine()) {
    throw std::invalid_argument("transform: only affine transformations are supported");
  }

  if (!dst_rect.isValid()) {
    throw std::invalid_argument("transform: dst_rect is invalid");
  }

  auto is_opaque_gray
      = [](QRgb rgba) { return qAlpha(rgba) == 0xff && qRed(rgba) == qBlue(rgba) && qRed(rgba) == qGreen(rgba); };
  switch (src.format()) {
    case QImage::Format_Invalid:
      return QImage();
    case QImage::Format_Indexed8:
    case QImage::Format_Mono:
    case QImage::Format_MonoLSB:
      if (src.allGray() && is_opaque_gray(outside_pixels.rgba())) {
        // The palette of src may be non-standard, so we create a GrayImage,
        // which is guaranteed to have a standard palette.
        GrayImage gray_src(src);
        GrayImage gray_dst(dst_rect.size());
        typedef uint32_t AccumType;
        transformGeneric<uint8_t, GrayColorMixer<AccumType>>(
            gray_src.data(), gray_src.stride(), src.size(), gray_dst.data(), gray_dst.stride(), xform, dst_rect,
            outside_pixels.grayLevel(), outside_pixels.flags(), min_mapping_area);

        fixDpiInPlace(gray_dst, xform);

        return gray_dst;
      }
    default:
      if (!src.hasAlphaChannel() && (qAlpha(outside_pixels.rgba()) == 0xff)) {
        const QImage src_rgb32(src.convertToFormat(QImage::Format_RGB32));
        badAllocIfNull(src_rgb32);
        QImage dst(dst_rect.size(), QImage::Format_RGB32);
        badAllocIfNull(dst);

        typedef uint32_t AccumType;
        transformGeneric<uint32_t, RgbColorMixer<AccumType>>(
            (const uint32_t*) src_rgb32.bits(), src_rgb32.bytesPerLine() / 4, src_rgb32.size(), (uint32_t*) dst.bits(),
            dst.bytesPerLine() / 4, xform, dst_rect, outside_pixels.rgb(), outside_pixels.flags(), min_mapping_area);

        fixDpiInPlace(dst, xform);

        return dst;
      } else {
        const QImage src_argb32(src.convertToFormat(QImage::Format_ARGB32));
        badAllocIfNull(src_argb32);
        QImage dst(dst_rect.size(), QImage::Format_ARGB32);
        badAllocIfNull(dst);

        typedef float AccumType;
        transformGeneric<uint32_t, ArgbColorMixer<AccumType>>(
            (const uint32_t*) src_argb32.bits(), src_argb32.bytesPerLine() / 4, src_argb32.size(),
            (uint32_t*) dst.bits(), dst.bytesPerLine() / 4, xform, dst_rect, outside_pixels.rgba(),
            outside_pixels.flags(), min_mapping_area);

        fixDpiInPlace(dst, xform);

        return dst;
      }
  }
}  // transform

GrayImage transformToGray(const QImage& src,
                          const QTransform& xform,
                          const QRect& dst_rect,
                          const OutsidePixels outside_pixels,
                          const QSizeF& min_mapping_area) {
  if (src.isNull() || dst_rect.isEmpty()) {
    return GrayImage();
  }

  if (!xform.isAffine()) {
    throw std::invalid_argument("transformToGray: only affine transformations are supported");
  }

  if (!dst_rect.isValid()) {
    throw std::invalid_argument("transformToGray: dst_rect is invalid");
  }

  const GrayImage gray_src(src);
  GrayImage dst(dst_rect.size());

  typedef unsigned AccumType;
  transformGeneric<uint8_t, GrayColorMixer<AccumType>>(gray_src.data(), gray_src.stride(), gray_src.size(), dst.data(),
                                                       dst.stride(), xform, dst_rect, outside_pixels.grayLevel(),
                                                       outside_pixels.flags(), min_mapping_area);

  fixDpiInPlace(dst, xform);

  return dst;
}
}  // namespace imageproc