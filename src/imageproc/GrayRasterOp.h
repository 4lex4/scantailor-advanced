// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef IMAGEPROC_GRAYRASTEROP_H_
#define IMAGEPROC_GRAYRASTEROP_H_

#include <QPoint>
#include <QRect>
#include <QSize>
#include <cassert>
#include <cstdint>
#include <stdexcept>
#include "GrayImage.h"
#include "Grayscale.h"

namespace imageproc {
/**
 * \brief Perform pixel-wise operations on two images.
 *
 * \param dst The destination image.  Changes will be written there.
 * \param src The source image.  May be the same as the destination image.
 *
 * The template argument is the operation to perform.  This is generally a
 * combination of several GRop* class templates, such as
 * GRopSubtract\<GRopSrc, GRopDst\>.
 */
template <typename GRop>
void grayRasterOp(GrayImage& dst, const GrayImage& src);

/**
 * \brief Raster operation that takes source pixels as they are.
 * \see grayRasterOp()
 */
class GRopSrc {
 public:
  static uint8_t transform(uint8_t src, uint8_t /*dst*/) { return src; }
};


/**
 * \brief Raster operation that takes destination pixels as they are.
 * \see grayRasterOp()
 */
class GRopDst {
 public:
  static uint8_t transform(uint8_t /*src*/, uint8_t dst) { return dst; }
};


/**
 * \brief Raster operation that inverts the gray level.
 * \see grayRasterOp()
 */
template <typename Arg>
class GRopInvert {
 public:
  static uint8_t transform(uint8_t src, uint8_t dst) { return uint8_t(0xff) - Arg::transform(src, dst); }
};


/**
 * \brief Raster operation that subtracts gray levels of Rhs from Lhs.
 *
 * The "Clipped" part of the name indicates that negative subtraction results
 * are turned into zero.
 *
 * \see grayRasterOp()
 */
template <typename Lhs, typename Rhs>
class GRopClippedSubtract {
 public:
  static uint8_t transform(uint8_t src, uint8_t dst) {
    const uint8_t lhs = Lhs::transform(src, dst);
    const uint8_t rhs = Rhs::transform(src, dst);

    return lhs > rhs ? lhs - rhs : uint8_t(0);
  }
};


/**
 * \brief Raster operation that subtracts gray levels of Rhs from Lhs.
 *
 * The "Unclipped" part of the name indicates that underflows aren't handled.
 * Negative results will appear as 256 - |negative_result|.
 *
 * \see grayRasterOp()
 */
template <typename Lhs, typename Rhs>
class GRopUnclippedSubtract {
 public:
  static uint8_t transform(uint8_t src, uint8_t dst) {
    const uint8_t lhs = Lhs::transform(src, dst);
    const uint8_t rhs = Rhs::transform(src, dst);

    return lhs - rhs;
  }
};


/**
 * \brief Raster operation that sums Rhs and Lhs gray levels.
 *
 * The "Clipped" part of the name indicates that overflow are clipped at 255.
 *
 * \see grayRasterOp()
 */
template <typename Lhs, typename Rhs>
class GRopClippedAdd {
 public:
  static uint8_t transform(uint8_t src, uint8_t dst) {
    const unsigned lhs = Lhs::transform(src, dst);
    const unsigned rhs = Rhs::transform(src, dst);
    const unsigned sum = lhs + rhs;

    return sum < 256 ? static_cast<uint8_t>(sum) : uint8_t(255);
  }
};


/**
 * \brief Raster operation that sums Rhs and Lhs gray levels.
 *
 * The "Unclipped" part of the name indicates that overflows aren't handled.
 * Results exceeding 255 will appear as result - 256.
 *
 * \see grayRasterOp()
 */
template <typename Lhs, typename Rhs>
class GRopUnclippedAdd {
 public:
  static uint8_t transform(uint8_t src, uint8_t dst) {
    const uint8_t lhs = Lhs::transform(src, dst);
    const uint8_t rhs = Rhs::transform(src, dst);

    return lhs + rhs;
  }
};


/**
 * \brief Raster operation that takes the darkest of its arguments.
 * \see grayRasterOp()
 */
template <typename Lhs, typename Rhs>
class GRopDarkest {
 public:
  static uint8_t transform(uint8_t src, uint8_t dst) {
    const uint8_t lhs = Lhs::transform(src, dst);
    const uint8_t rhs = Rhs::transform(src, dst);

    return lhs < rhs ? lhs : rhs;
  }
};


/**
 * \brief Raster operation that takes the lightest of its arguments.
 * \see grayRasterOp()
 */
template <typename Lhs, typename Rhs>
class GRopLightest {
 public:
  static uint8_t transform(uint8_t src, uint8_t dst) {
    const uint8_t lhs = Lhs::transform(src, dst);
    const uint8_t rhs = Rhs::transform(src, dst);

    return lhs > rhs ? lhs : rhs;
  }
};


template <typename GRop>
void grayRasterOp(GrayImage& dst, const GrayImage& src) {
  if (dst.isNull() || src.isNull()) {
    throw std::invalid_argument("grayRasterOp: can't operate on null images");
  }

  if (src.size() != dst.size()) {
    throw std::invalid_argument("grayRasterOp: images sizes are not the same");
  }

  const uint8_t* src_line = src.data();
  uint8_t* dst_line = dst.data();
  const int src_stride = src.stride();
  const int dst_stride = dst.stride();

  const int width = src.width();
  const int height = src.height();

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      dst_line[x] = GRop::transform(src_line[x], dst_line[x]);
    }
    src_line += src_stride;
    dst_line += dst_stride;
  }
}
}  // namespace imageproc
#endif  // ifndef IMAGEPROC_GRAYRASTEROP_H_
