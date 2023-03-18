// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "Scale.h"

#include <cassert>
#include <stdexcept>

#include "GrayImage.h"

namespace imageproc {
/**
 * This is an optimized implementation for the case when every destination
 * pixel maps exactly to a M x N block of source pixels.
 */
static GrayImage scaleDownIntGrayToGray(const GrayImage& src, const QSize& dstSize) {
  const int sw = src.width();
  const int sh = src.height();
  const int dw = dstSize.width();
  const int dh = dstSize.height();

  const int xscale = sw / dw;
  const int yscale = sh / dh;
  const int totalArea = xscale * yscale;

  GrayImage dst(dstSize);

  const uint8_t* srcLine = src.data();
  uint8_t* dstLine = dst.data();
  const int srcStride = src.stride();
  const int srcStrideScaled = srcStride * yscale;
  const int dstStride = dst.stride();

  int sy = 0;
  int dy = 0;
  for (; dy < dh; ++dy, sy += yscale) {
    int sx = 0;
    int dx = 0;
    for (; dx < dw; ++dx, sx += xscale) {
      unsigned grayLevel = 0;
      const uint8_t* psrc = srcLine + sx;

      for (int i = 0; i < yscale; ++i, psrc += srcStride) {
        for (int j = 0; j < xscale; ++j) {
          grayLevel += psrc[j];
        }
      }

      const unsigned pixValue = (grayLevel + (totalArea >> 1)) / totalArea;
      assert(pixValue < 256);
      dstLine[dx] = static_cast<uint8_t>(pixValue);
    }

    srcLine += srcStrideScaled;
    dstLine += dstStride;
  }
  return dst;
}  // scaleDownIntGrayToGray

/**
 * This is an optimized implementation for the case when every destination
 * pixel maps to a single source pixel (possibly to a part of it).
 */
static GrayImage scaleUpIntGrayToGray(const GrayImage& src, const QSize& dstSize) {
  const int sw = src.width();
  const int sh = src.height();
  const int dw = dstSize.width();
  const int dh = dstSize.height();

  const int xscale = dw / sw;
  const int yscale = dh / sh;

  GrayImage dst(dstSize);

  const uint8_t* srcLine = src.data();
  uint8_t* dstLine = dst.data();
  const int srcStride = src.stride();
  const int dstStride = dst.stride();
  const int dstStrideScaled = dstStride * yscale;

  int sy = 0;
  int dy = 0;
  for (; dy < dh; ++sy, dy += xscale) {
    int sx = 0;
    int dx = 0;

    for (; dx < dw; ++sx, dx += xscale) {
      uint8_t* pdst = dstLine + dx;

      for (int i = 0; i < yscale; ++i, pdst += dstStride) {
        for (int j = 0; j < xscale; ++j) {
          pdst[j] = srcLine[sx];
        }
      }
    }

    srcLine += srcStride;
    dstLine += dstStrideScaled;
  }
  return dst;
}  // scaleUpIntGrayToGray

/**
 * This function is used to calculate the ratio for going
 * from \p dst to \p src multiplied by 32, so that
 * \code
 * int(ratio * (dst_limit - 1)) / 32 < src_limit - 1
 * \endcode
 */
static double calc32xRatio1(const int dst, const int src) {
  assert(dst > 0);
  assert(src > 0);

  int src32 = src << 5;
  double ratio = (double) src32 / dst;
  while ((int(ratio * (dst - 1)) >> 5) + 1 >= src) {
    --src32;
    ratio = (double) src32 / dst;
  }
  return ratio;
}

/**
 * This is an optimized implementation for the case when
 * the destination image is larger than the source image both
 * horizontally and vertically.
 */
static GrayImage scaleUpGrayToGray(const GrayImage& src, const QSize& dstSize) {
  const int sw = src.width();
  const int sh = src.height();
  const int dw = dstSize.width();
  const int dh = dstSize.height();

  const double dx2sx32 = calc32xRatio1(dw, sw);
  const double dy2sy32 = calc32xRatio1(dh, sh);

  GrayImage dst(dstSize);

  const uint8_t* const srcData = src.data();
  uint8_t* dstLine = dst.data();
  const int srcStride = src.stride();
  const int dstStride = dst.stride();

  for (int dy = 0; dy < dh; ++dy, dstLine += dstStride) {
    const auto sy32 = (int) (dy * dy2sy32);
    const int sy = sy32 >> 5;
    const unsigned topFraction = 32 - (sy32 & 31);
    const unsigned bottomFraction = sy32 & 31;
    assert(sy + 1 < sh);  // calc32xRatio1() ensures that.
    const uint8_t* srcLine = srcData + sy * srcStride;

    for (int dx = 0; dx < dw; ++dx) {
      const auto sx32 = (int) (dx * dx2sx32);
      const int sx = sx32 >> 5;
      const unsigned leftFraction = 32 - (sx32 & 31);
      const unsigned rightFraction = sx32 & 31;
      assert(sx + 1 < sw);  // calc32xRatio1() ensures that.
      unsigned grayLevel = 0;

      const uint8_t* psrc = srcLine + sx;
      grayLevel += *psrc * leftFraction * topFraction;
      ++psrc;
      grayLevel += *psrc * rightFraction * topFraction;
      psrc += srcStride;
      grayLevel += *psrc * rightFraction * bottomFraction;
      --psrc;
      grayLevel += *psrc * leftFraction * bottomFraction;

      const unsigned totalArea = 32 * 32;
      const unsigned pixValue = (grayLevel + (totalArea >> 1)) / totalArea;
      assert(pixValue < 256);
      dstLine[dx] = static_cast<uint8_t>(pixValue);
    }
  }
  return dst;
}  // scaleUpGrayToGray

/**
 * This function is used to calculate the ratio for going
 * from \p dst to \p src multiplied by 32, so that
 * \code
 * (int(ratio * dst_limit) - 1) / 32 < src_limit
 * \endcode
 */
static double calc32xRatio2(const int dst, const int src) {
  assert(dst > 0);
  assert(src > 0);

  int src32 = src << 5;
  double ratio = (double) src32 / dst;
  while ((int(ratio * dst) - 1) >> 5 >= src) {
    --src32;
    ratio = (double) src32 / dst;
  }
  return ratio;
}

/**
 * This is a generic implementation of the scaling algorithm.
 */
static GrayImage scaleGrayToGray(const GrayImage& src, const QSize& dstSize) {
  const int sw = src.width();
  const int sh = src.height();
  const int dw = dstSize.width();
  const int dh = dstSize.height();

  // Try versions optimized for a particular case.
  if ((sw == dw) && (sh == dh)) {
    return src;
  } else if ((sw % dw == 0) && (sh % dh == 0)) {
    return scaleDownIntGrayToGray(src, dstSize);
  } else if ((dw % sw == 0) && (dh % sh == 0)) {
    return scaleUpIntGrayToGray(src, dstSize);
  } else if ((dw > sw) && (dh > sh)) {
    return scaleUpGrayToGray(src, dstSize);
  }

  const double dx2sx32 = calc32xRatio2(dw, sw);
  const double dy2sy32 = calc32xRatio2(dh, sh);

  GrayImage dst(dstSize);

  const uint8_t* const srcData = src.data();
  uint8_t* dstLine = dst.data();
  const int srcStride = src.stride();
  const int dstStride = dst.stride();

  int sy32bottom = 0;
  for (int dy1 = 1; dy1 <= dh; ++dy1, dstLine += dstStride) {
    const int sy32top = sy32bottom;
    sy32bottom = (int) (dy1 * dy2sy32);
    const int sytop = sy32top >> 5;
    const int sybottom = (sy32bottom - 1) >> 5;
    const unsigned topFraction = 32 - (sy32top & 31);
    const unsigned bottomFraction = sy32bottom - (sybottom << 5);
    assert(sybottom < sh);  // calc32xRatio2() ensures that.
    const unsigned topArea = topFraction << 5;
    const unsigned bottomArea = bottomFraction << 5;

    const uint8_t* const srcLineConst = srcData + sytop * srcStride;

    int sx32right = 0;
    for (int dx = 0; dx < dw; ++dx) {
      const int sx32left = sx32right;
      sx32right = (int) ((dx + 1) * dx2sx32);
      const int sxleft = sx32left >> 5;
      const int sxright = (sx32right - 1) >> 5;
      const unsigned leftFraction = 32 - (sx32left & 31);
      const unsigned rightFraction = sx32right - (sxright << 5);
      assert(sxright < sw);  // calc32xRatio2() ensures that.
      const uint8_t* srcLine = srcLineConst;
      unsigned grayLevel = 0;

      if (sytop == sybottom) {
        if (sxleft == sxright) {
          // dst pixel maps to a single src pixel
          dstLine[dx] = srcLine[sxleft];
          continue;
        } else {
          // dst pixel maps to a horizontal line of src pixels
          const unsigned vertFraction = sy32bottom - sy32top;
          const unsigned leftArea = vertFraction * leftFraction;
          const unsigned middleArea = vertFraction << 5;
          const unsigned rightArea = vertFraction * rightFraction;

          grayLevel += srcLine[sxleft] * leftArea;

          for (int sx = sxleft + 1; sx < sxright; ++sx) {
            grayLevel += srcLine[sx] * middleArea;
          }

          grayLevel += srcLine[sxright] * rightArea;
        }
      } else if (sxleft == sxright) {
        // dst pixel maps to a vertical line of src pixels
        const unsigned horFraction = sx32right - sx32left;
        const unsigned topArea = horFraction * topFraction;
        const unsigned middleArea = horFraction << 5;
        const unsigned bottomArea = horFraction * bottomFraction;

        grayLevel += srcLine[sxleft] * topArea;

        srcLine += srcStride;

        for (int sy = sytop + 1; sy < sybottom; ++sy) {
          grayLevel += srcLine[sxleft] * middleArea;
          srcLine += srcStride;
        }

        grayLevel += srcLine[sxleft] * bottomArea;
      } else {
        // dst pixel maps to a block of src pixels
        const unsigned leftArea = leftFraction << 5;
        const unsigned rightArea = rightFraction << 5;
        const unsigned topleftArea = topFraction * leftFraction;
        const unsigned toprightArea = topFraction * rightFraction;
        const unsigned bottomleftArea = bottomFraction * leftFraction;
        const unsigned bottomrightArea = bottomFraction * rightFraction;

        // process the top-left corner
        grayLevel += srcLine[sxleft] * topleftArea;

        // process the top line (without corners)
        for (int sx = sxleft + 1; sx < sxright; ++sx) {
          grayLevel += srcLine[sx] * topArea;
        }

        // process the top-right corner
        grayLevel += srcLine[sxright] * toprightArea;

        srcLine += srcStride;
        // process middle lines
        for (int sy = sytop + 1; sy < sybottom; ++sy) {
          grayLevel += srcLine[sxleft] * leftArea;

          for (int sx = sxleft + 1; sx < sxright; ++sx) {
            grayLevel += srcLine[sx] << (5 + 5);
          }

          grayLevel += srcLine[sxright] * rightArea;

          srcLine += srcStride;
        }

        // process bottom-left corner
        grayLevel += srcLine[sxleft] * bottomleftArea;

        // process the bottom line (without corners)
        for (int sx = sxleft + 1; sx < sxright; ++sx) {
          grayLevel += srcLine[sx] * bottomArea;
        }
        // process the bottom-right corner
        grayLevel += srcLine[sxright] * bottomrightArea;
      }

      const unsigned totalArea = (sy32bottom - sy32top) * (sx32right - sx32left);
      const unsigned pixValue = (grayLevel + (totalArea >> 1)) / totalArea;
      assert(pixValue < 256);
      dstLine[dx] = static_cast<uint8_t>(pixValue);
    }
  }
  return dst;
}  // scaleGrayToGray

GrayImage scaleToGray(const GrayImage& src, const QSize& dstSize) {
  if (src.isNull()) {
    return src;
  }

  if (!dstSize.isValid()) {
    throw std::invalid_argument("scaleToGray: dstSize is invalid");
  }

  if (dstSize.isEmpty()) {
    return GrayImage();
  }
  return scaleGrayToGray(src, dstSize);
}
}  // namespace imageproc
