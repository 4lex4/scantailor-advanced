// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "OrthogonalRotation.h"
#include "BinaryImage.h"
#include "RasterOp.h"

namespace imageproc {
static inline uint32_t mask(int x) {
  return (uint32_t(1) << 31) >> (x % 32);
}

static BinaryImage rotate0(const BinaryImage& src, const QRect& src_rect) {
  if (src_rect == src.rect()) {
    return src;
  }

  BinaryImage dst(src_rect.width(), src_rect.height());
  rasterOp<RopSrc>(dst, dst.rect(), src, src_rect.topLeft());

  return dst;
}

static BinaryImage rotate90(const BinaryImage& src, const QRect& src_rect) {
  const int dst_w = src_rect.height();
  const int dst_h = src_rect.width();
  BinaryImage dst(dst_w, dst_h);
  dst.fill(WHITE);
  const int src_wpl = src.wordsPerLine();
  const int dst_wpl = dst.wordsPerLine();
  const uint32_t* const src_data = src.data() + src_rect.bottom() * src_wpl;
  uint32_t* dst_line = dst.data();

  /*
   *   dst
   *  ----->
   * ^
   * | src
   * |
   */

  for (int dst_y = 0; dst_y < dst_h; ++dst_y) {
    const int src_x = src_rect.left() + dst_y;
    const uint32_t* src_pword = src_data + src_x / 32;
    const uint32_t src_mask = mask(src_x);

    for (int dst_x = 0; dst_x < dst_w; ++dst_x) {
      if (*src_pword & src_mask) {
        dst_line[dst_x / 32] |= mask(dst_x);
      }
      src_pword -= src_wpl;
    }

    dst_line += dst_wpl;
  }

  return dst;
}

static BinaryImage rotate180(const BinaryImage& src, const QRect& src_rect) {
  const int dst_w = src_rect.width();
  const int dst_h = src_rect.height();
  BinaryImage dst(dst_w, dst_h);
  dst.fill(WHITE);
  const int src_wpl = src.wordsPerLine();
  const int dst_wpl = dst.wordsPerLine();
  const uint32_t* src_line = src.data() + src_rect.bottom() * src_wpl;
  uint32_t* dst_line = dst.data();

  /*
   *  dst
   * ----->
   * <-----
   *  src
   */

  for (int dst_y = 0; dst_y < dst_h; ++dst_y) {
    int src_x = src_rect.right();
    for (int dst_x = 0; dst_x < dst_w; --src_x, ++dst_x) {
      if (src_line[src_x / 32] & mask(src_x)) {
        dst_line[dst_x / 32] |= mask(dst_x);
      }
    }

    src_line -= src_wpl;
    dst_line += dst_wpl;
  }

  return dst;
}

static BinaryImage rotate270(const BinaryImage& src, const QRect& src_rect) {
  const int dst_w = src_rect.height();
  const int dst_h = src_rect.width();
  BinaryImage dst(dst_w, dst_h);
  dst.fill(WHITE);
  const int src_wpl = src.wordsPerLine();
  const int dst_wpl = dst.wordsPerLine();
  const uint32_t* const src_data = src.data() + src_rect.top() * src_wpl;
  uint32_t* dst_line = dst.data();

  /*
   *  dst
   * ----->
   *       |
   *   src |
   *       v
   */

  for (int dst_y = 0; dst_y < dst_h; ++dst_y) {
    const int src_x = src_rect.right() - dst_y;
    const uint32_t* src_pword = src_data + src_x / 32;
    const uint32_t src_mask = mask(src_x);

    for (int dst_x = 0; dst_x < dst_w; ++dst_x) {
      if (*src_pword & src_mask) {
        dst_line[dst_x / 32] |= mask(dst_x);
      }
      src_pword += src_wpl;
    }

    dst_line += dst_wpl;
  }

  return dst;
}

BinaryImage orthogonalRotation(const BinaryImage& src, const QRect& src_rect, const int degrees) {
  if (src.isNull() || src_rect.isNull()) {
    return BinaryImage();
  }

  if (src_rect.intersected(src.rect()) != src_rect) {
    throw std::invalid_argument("orthogonalRotation: invalid src_rect");
  }

  switch (degrees % 360) {
    case 0:
      return rotate0(src, src_rect);
    case 90:
    case -270:
      return rotate90(src, src_rect);
    case 180:
    case -180:
      return rotate180(src, src_rect);
    case 270:
    case -90:
      return rotate270(src, src_rect);
    default:
      throw std::invalid_argument("orthogonalRotation: invalid angle");
  }
}

BinaryImage orthogonalRotation(const BinaryImage& src, const int degrees) {
  return orthogonalRotation(src, src.rect(), degrees);
}
}  // namespace imageproc