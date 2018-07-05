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

#include "RasterDewarper.h"
#include <QDebug>
#include <cmath>
#include "CylindricalSurfaceDewarper.h"
#include "imageproc/ColorMixer.h"
#include "imageproc/GrayImage.h"

#define INTERP_NONE 0
#define INTERP_BILLINEAR 1
#define INTERP_AREA_MAPPING 2
#define INTERPOLATION_METHOD INTERP_AREA_MAPPING

using namespace imageproc;

namespace dewarping {
namespace {
#if INTERPOLATION_METHOD == INTERP_NONE
template <typename ColorMixer, typename PixelType>
void dewarpGeneric(const PixelType* const src_data,
                   const QSize src_size,
                   const int src_stride,
                   PixelType* const dst_data,
                   const QSize dst_size,
                   const int dst_stride,
                   const CylindricalSurfaceDewarper& distortion_model,
                   const QRectF& model_domain,
                   const PixelType bg_color) {
  const int src_width = src_size.width();
  const int src_height = src_size.height();
  const int dst_width = dst_size.width();
  const int dst_height = dst_size.height();

  CylindricalSurfaceDewarper::State state;

  const double model_domain_left = model_domain.left();
  const double model_x_scale = 1.0 / (model_domain.right() - model_domain.left());

  const float model_domain_top = model_domain.top();
  const float model_y_scale = 1.0 / (model_domain.bottom() - model_domain.top());

  for (int dst_x = 0; dst_x < dst_width; ++dst_x) {
    const double model_x = (dst_x - model_domain_left) * model_x_scale;
    const CylindricalSurfaceDewarper::Generatrix generatrix(distortion_model.mapGeneratrix(model_x, state));

    const HomographicTransform<1, float> homog(generatrix.pln2img.mat());
    const Vec2f origin(generatrix.imgLine.p1());
    const Vec2f vec(generatrix.imgLine.p2() - generatrix.imgLine.p1());
    for (int dst_y = 0; dst_y < dst_height; ++dst_y) {
      const float model_y = (float(dst_y) - model_domain_top) * model_y_scale;
      const Vec2f src_pt(origin + vec * homog(model_y));
      const int src_x = qRound(src_pt[0]);
      const int src_y = qRound(src_pt[1]);
      if ((src_x < 0) || (src_x >= src_width) || (src_y < 0) || (src_y >= src_height)) {
        dst_data[dst_y * dst_stride + dst_x] = bg_color;
        continue;
      }

      dst_data[dst_y * dst_stride + dst_x] = src_data[src_y * src_stride + src_x];
    }
  }
}  // dewarpGeneric

#elif INTERPOLATION_METHOD == INTERP_BILLINEAR
template <typename ColorMixer, typename PixelType>
void dewarpGeneric(const PixelType* const src_data,
                   const QSize src_size,
                   const int src_stride,
                   PixelType* const dst_data,
                   const QSize dst_size,
                   const int dst_stride,
                   const CylindricalSurfaceDewarper& distortion_model,
                   const QRectF& model_domain,
                   const PixelType bg_color) {
  const int src_width = src_size.width();
  const int src_height = src_size.height();
  const int dst_width = dst_size.width();
  const int dst_height = dst_size.height();

  CylindricalSurfaceDewarper::State state;

  const double model_domain_left = model_domain.left() - 0.5f;
  const double model_x_scale = 1.0 / (model_domain.right() - model_domain.left());

  const float model_domain_top = model_domain.top() - 0.5f;
  const float model_y_scale = 1.0 / (model_domain.bottom() - model_domain.top());

  for (int dst_x = 0; dst_x < dst_width; ++dst_x) {
    const double model_x = (dst_x - model_domain_left) * model_x_scale;
    const CylindricalSurfaceDewarper::Generatrix generatrix(distortion_model.mapGeneratrix(model_x, state));

    const HomographicTransform<1, float> homog(generatrix.pln2img.mat());
    const Vec2f origin(generatrix.imgLine.p1());
    const Vec2f vec(generatrix.imgLine.p2() - generatrix.imgLine.p1());
    for (int dst_y = 0; dst_y < dst_height; ++dst_y) {
      const float model_y = ((float) dst_y - model_domain_top) * model_y_scale;
      const Vec2f src_pt(origin + vec * homog(model_y));

      const int src_x0 = (int) std::floor(src_pt[0] - 0.5f);
      const int src_y0 = (int) std::floor(src_pt[1] - 0.5f);
      const int src_x1 = src_x0 + 1;
      const int src_y1 = src_y0 + 1;
      const float x = src_pt[0] - src_x0;
      const float y = src_pt[1] - src_y0;

      PixelType tl_color = bg_color;
      if ((src_x0 >= 0) && (src_x0 < src_width) && (src_y0 >= 0) && (src_y0 < src_height)) {
        tl_color = src_data[src_y0 * src_stride + src_x0];
      }

      PixelType tr_color = bg_color;
      if ((src_x1 >= 0) && (src_x1 < src_width) && (src_y0 >= 0) && (src_y0 < src_height)) {
        tr_color = src_data[src_y0 * src_stride + src_x1];
      }

      PixelType bl_color = bg_color;
      if ((src_x0 >= 0) && (src_x0 < src_width) && (src_y1 >= 0) && (src_y1 < src_height)) {
        bl_color = src_data[src_y1 * src_stride + src_x0];
      }

      PixelType br_color = bg_color;
      if ((src_x1 >= 0) && (src_x1 < src_width) && (src_y1 >= 0) && (src_y1 < src_height)) {
        br_color = src_data[src_y1 * src_stride + src_x1];
      }

      ColorMixer mixer;
      mixer.add(tl_color, (1.5f - y) * (1.5f - x));
      mixer.add(tr_color, (1.5f - y) * (x - 0.5f));
      mixer.add(bl_color, (y - 0.5f) * (1.5f - x));
      mixer.add(br_color, (y - 0.5f) * (x - 0.5f));
      dst_data[dst_y * dst_stride + dst_x] = mixer.mix(1.0f);
    }
  }
}  // dewarpGeneric

#elif INTERPOLATION_METHOD == INTERP_AREA_MAPPING

template <typename ColorMixer, typename PixelType>
void areaMapGeneratrix(const PixelType* const src_data,
                       const QSize src_size,
                       const int src_stride,
                       PixelType* p_dst,
                       const QSize dst_size,
                       const int dst_stride,
                       const PixelType bg_color,
                       const std::vector<Vec2f>& prev_grid_column,
                       const std::vector<Vec2f>& next_grid_column) {
  const int sw = src_size.width();
  const int sh = src_size.height();
  const int dst_height = dst_size.height();

  const Vec2f* src_left_points = &prev_grid_column[0];
  const Vec2f* src_right_points = &next_grid_column[0];

  Vec2f f_src32_quad[4];

  for (int dst_y = 0; dst_y < dst_height; ++dst_y) {
    // Take a mid-point of each edge, pre-multiply by 32,
    // write the result to f_src32_quad. 16 comes from 32*0.5
    f_src32_quad[0] = 16.0f * (src_left_points[0] + src_right_points[0]);
    f_src32_quad[1] = 16.0f * (src_right_points[0] + src_right_points[1]);
    f_src32_quad[2] = 16.0f * (src_right_points[1] + src_left_points[1]);
    f_src32_quad[3] = 16.0f * (src_left_points[0] + src_left_points[1]);
    ++src_left_points;
    ++src_right_points;

    // Calculate the bounding box of src_quad.

    float f_src32_left = f_src32_quad[0][0];
    float f_src32_top = f_src32_quad[0][1];
    float f_src32_right = f_src32_left;
    float f_src32_bottom = f_src32_top;

    for (int i = 1; i < 4; ++i) {
      const Vec2f pt(f_src32_quad[i]);
      if (pt[0] < f_src32_left) {
        f_src32_left = pt[0];
      } else if (pt[0] > f_src32_right) {
        f_src32_right = pt[0];
      }
      if (pt[1] < f_src32_top) {
        f_src32_top = pt[1];
      } else if (pt[1] > f_src32_bottom) {
        f_src32_bottom = pt[1];
      }
    }

    if ((f_src32_top < -32.0f * 10000.0f) || (f_src32_left < -32.0f * 10000.0f)
        || (f_src32_bottom > 32.0f * (float(sh) + 10000.f)) || (f_src32_right > 32.0f * (float(sw) + 10000.f))) {
      // This helps to prevent integer overflows.
      *p_dst = bg_color;
      p_dst += dst_stride;
      continue;
    }

    // Note: the code below is more or less the same as in transformGeneric()
    // in imageproc/Transform.cpp

    // Note that without using std::floor() and std::ceil()
    // we can't guarantee that src_bottom >= src_top
    // and src_right >= src_left.
    auto src32_left = (int) std::floor(f_src32_left);
    auto src32_right = (int) std::ceil(f_src32_right);
    auto src32_top = (int) std::floor(f_src32_top);
    auto src32_bottom = (int) std::ceil(f_src32_bottom);
    int src_left = src32_left >> 5;
    int src_right = (src32_right - 1) >> 5;  // inclusive
    int src_top = src32_top >> 5;
    int src_bottom = (src32_bottom - 1) >> 5;  // inclusive
    assert(src_bottom >= src_top);
    assert(src_right >= src_left);

    if ((src_bottom < 0) || (src_right < 0) || (src_left >= sw) || (src_top >= sh)) {
      // Completely outside of src image.
      *p_dst = bg_color;
      p_dst += dst_stride;
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

    ColorMixer mixer;
    // if (weak_background) {
    // background_area = 0;
    // } else {
    mixer.add(bg_color, background_area);
    // }

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
      *p_dst = bg_color;
      p_dst += dst_stride;
      continue;
    }

    const PixelType* src_line = &src_data[src_top * src_stride];

    if (src_top == src_bottom) {
      if (src_left == src_right) {
        // dst pixel maps to a single src pixel
        const PixelType c = src_line[src_left];
        if (background_area == 0) {
          // common case optimization
          *p_dst = c;
          p_dst += dst_stride;
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

    *p_dst = mixer.mix(src_area + background_area);
    p_dst += dst_stride;
  }
}  // areaMapGeneratrix

template <typename ColorMixer, typename PixelType>
void dewarpGeneric(const PixelType* const src_data,
                   const QSize src_size,
                   const int src_stride,
                   PixelType* const dst_data,
                   const QSize dst_size,
                   const int dst_stride,
                   const CylindricalSurfaceDewarper& distortion_model,
                   const QRectF& model_domain,
                   const PixelType bg_color) {
  const int src_width = src_size.width();
  const int src_height = src_size.height();
  const int dst_width = dst_size.width();
  const int dst_height = dst_size.height();

  CylindricalSurfaceDewarper::State state;

  const double model_domain_left = model_domain.left();
  const double model_x_scale = 1.0 / (model_domain.right() - model_domain.left());

  const auto model_domain_top = static_cast<const float>(model_domain.top());
  const auto model_y_scale = static_cast<const float>(1.0 / (model_domain.bottom() - model_domain.top()));

  std::vector<Vec2f> prev_grid_column(dst_height + 1);
  std::vector<Vec2f> next_grid_column(dst_height + 1);

  for (int dst_x = 0; dst_x <= dst_width; ++dst_x) {
    const double model_x = (dst_x - model_domain_left) * model_x_scale;
    const CylindricalSurfaceDewarper::Generatrix generatrix(distortion_model.mapGeneratrix(model_x, state));

    const HomographicTransform<1, float> homog(generatrix.pln2img.mat());
    const Vec2f origin(generatrix.imgLine.p1());
    const Vec2f vec(generatrix.imgLine.p2() - generatrix.imgLine.p1());
    for (int dst_y = 0; dst_y <= dst_height; ++dst_y) {
      const float model_y = (float(dst_y) - model_domain_top) * model_y_scale;
      next_grid_column[dst_y] = origin + vec * homog(model_y);
    }

    if (dst_x != 0) {
      areaMapGeneratrix<ColorMixer, PixelType>(src_data, src_size, src_stride, dst_data + dst_x - 1, dst_size,
                                               dst_stride, bg_color, prev_grid_column, next_grid_column);
    }

    prev_grid_column.swap(next_grid_column);
  }
}  // dewarpGeneric
#endif  // INTERPOLATION_METHOD
#if INTERPOLATION_METHOD == INTERP_BILLINEAR
typedef float MixingWeight;
#else
typedef unsigned MixingWeight;
#endif

QImage dewarpGrayscale(const QImage& src,
                       const QSize& dst_size,
                       const CylindricalSurfaceDewarper& distortion_model,
                       const QRectF& model_domain,
                       const QColor& bg_color) {
  GrayImage dst(dst_size);
  const auto bg_sample = static_cast<const uint8_t>(qGray(bg_color.rgb()));
  dst.fill(bg_sample);
  dewarpGeneric<GrayColorMixer<MixingWeight>, uint8_t>(src.bits(), src.size(), src.bytesPerLine(), dst.data(), dst_size,
                                                       dst.stride(), distortion_model, model_domain, bg_sample);

  return dst.toQImage();
}

QImage dewarpRgb(const QImage& src,
                 const QSize& dst_size,
                 const CylindricalSurfaceDewarper& distortion_model,
                 const QRectF& model_domain,
                 const QColor& bg_color) {
  QImage dst(dst_size, QImage::Format_RGB32);
  dst.fill(bg_color.rgb());
  dewarpGeneric<RgbColorMixer<MixingWeight>, uint32_t>((const uint32_t*) src.bits(), src.size(), src.bytesPerLine() / 4,
                                                       (uint32_t*) dst.bits(), dst_size, dst.bytesPerLine() / 4,
                                                       distortion_model, model_domain, bg_color.rgb());

  return dst;
}

QImage dewarpArgb(const QImage& src,
                  const QSize& dst_size,
                  const CylindricalSurfaceDewarper& distortion_model,
                  const QRectF& model_domain,
                  const QColor& bg_color) {
  QImage dst(dst_size, QImage::Format_ARGB32);
  dst.fill(bg_color.rgba());
  dewarpGeneric<ArgbColorMixer<MixingWeight>, uint32_t>(
      (const uint32_t*) src.bits(), src.size(), src.bytesPerLine() / 4, (uint32_t*) dst.bits(), dst_size,
      dst.bytesPerLine() / 4, distortion_model, model_domain, bg_color.rgba());

  return dst;
}
}  // namespace

QImage RasterDewarper::dewarp(const QImage& src,
                              const QSize& dst_size,
                              const CylindricalSurfaceDewarper& distortion_model,
                              const QRectF& model_domain,
                              const QColor& bg_color) {
  if (model_domain.isEmpty()) {
    throw std::invalid_argument("RasterDewarper: model_domain is empty.");
  }

  switch (src.format()) {
    case QImage::Format_Invalid:
      return QImage();
    case QImage::Format_RGB32:
      return dewarpRgb(src, dst_size, distortion_model, model_domain, bg_color);
    case QImage::Format_ARGB32:
      return dewarpArgb(src, dst_size, distortion_model, model_domain, bg_color);
    case QImage::Format_Indexed8:
      if (src.isGrayscale()) {
        return dewarpGrayscale(src, dst_size, distortion_model, model_domain, bg_color);
      } else if (src.allGray()) {
        // Only shades of gray but non-standard palette.
        return dewarpGrayscale(GrayImage(src).toQImage(), dst_size, distortion_model, model_domain, bg_color);
      }
      break;
    case QImage::Format_Mono:
    case QImage::Format_MonoLSB:
      if (src.allGray()) {
        return dewarpGrayscale(GrayImage(src).toQImage(), dst_size, distortion_model, model_domain, bg_color);
      }
      break;
    default:;
  }
  // Generic case: convert to either RGB32 or ARGB32.
  if (src.hasAlphaChannel()) {
    return dewarpArgb(src.convertToFormat(QImage::Format_ARGB32), dst_size, distortion_model, model_domain, bg_color);
  } else {
    return dewarpRgb(src.convertToFormat(QImage::Format_RGB32), dst_size, distortion_model, model_domain, bg_color);
  }
}  // RasterDewarper::dewarp
}  // namespace dewarping