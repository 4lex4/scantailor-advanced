// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "OrthogonalRotation.h"

#include "BinaryImage.h"
#include "RasterOp.h"

namespace imageproc {
static inline uint32_t mask(int x) {
  return (uint32_t(1) << 31) >> (x % 32);
}

static BinaryImage rotate0(const BinaryImage& src, const QRect& srcRect) {
  if (srcRect == src.rect()) {
    return src;
  }

  BinaryImage dst(srcRect.width(), srcRect.height());
  rasterOp<RopSrc>(dst, dst.rect(), src, srcRect.topLeft());
  return dst;
}

static BinaryImage rotate90(const BinaryImage& src, const QRect& srcRect) {
  const int dstW = srcRect.height();
  const int dstH = srcRect.width();
  BinaryImage dst(dstW, dstH);
  dst.fill(WHITE);
  const int srcWpl = src.wordsPerLine();
  const int dstWpl = dst.wordsPerLine();
  const uint32_t* const srcData = src.data() + srcRect.bottom() * srcWpl;
  uint32_t* dstLine = dst.data();

  /*
   *   dst
   *  ----->
   * ^
   * | src
   * |
   */

  for (int dstY = 0; dstY < dstH; ++dstY) {
    const int srcX = srcRect.left() + dstY;
    const uint32_t* srcPword = srcData + srcX / 32;
    const uint32_t srcMask = mask(srcX);

    for (int dstX = 0; dstX < dstW; ++dstX) {
      if (*srcPword & srcMask) {
        dstLine[dstX / 32] |= mask(dstX);
      }
      srcPword -= srcWpl;
    }

    dstLine += dstWpl;
  }
  return dst;
}

static BinaryImage rotate180(const BinaryImage& src, const QRect& srcRect) {
  const int dstW = srcRect.width();
  const int dstH = srcRect.height();
  BinaryImage dst(dstW, dstH);
  dst.fill(WHITE);
  const int srcWpl = src.wordsPerLine();
  const int dstWpl = dst.wordsPerLine();
  const uint32_t* srcLine = src.data() + srcRect.bottom() * srcWpl;
  uint32_t* dstLine = dst.data();

  /*
   *  dst
   * ----->
   * <-----
   *  src
   */

  for (int dstY = 0; dstY < dstH; ++dstY) {
    int srcX = srcRect.right();
    for (int dstX = 0; dstX < dstW; --srcX, ++dstX) {
      if (srcLine[srcX / 32] & mask(srcX)) {
        dstLine[dstX / 32] |= mask(dstX);
      }
    }

    srcLine -= srcWpl;
    dstLine += dstWpl;
  }
  return dst;
}

static BinaryImage rotate270(const BinaryImage& src, const QRect& srcRect) {
  const int dstW = srcRect.height();
  const int dstH = srcRect.width();
  BinaryImage dst(dstW, dstH);
  dst.fill(WHITE);
  const int srcWpl = src.wordsPerLine();
  const int dstWpl = dst.wordsPerLine();
  const uint32_t* const srcData = src.data() + srcRect.top() * srcWpl;
  uint32_t* dstLine = dst.data();

  /*
   *  dst
   * ----->
   *       |
   *   src |
   *       v
   */

  for (int dstY = 0; dstY < dstH; ++dstY) {
    const int srcX = srcRect.right() - dstY;
    const uint32_t* srcPword = srcData + srcX / 32;
    const uint32_t srcMask = mask(srcX);

    for (int dstX = 0; dstX < dstW; ++dstX) {
      if (*srcPword & srcMask) {
        dstLine[dstX / 32] |= mask(dstX);
      }
      srcPword += srcWpl;
    }

    dstLine += dstWpl;
  }
  return dst;
}

BinaryImage orthogonalRotation(const BinaryImage& src, const QRect& srcRect, const int degrees) {
  if (src.isNull() || srcRect.isNull()) {
    return BinaryImage();
  }

  if (srcRect.intersected(src.rect()) != srcRect) {
    throw std::invalid_argument("orthogonalRotation: invalid srcRect");
  }

  switch (degrees % 360) {
    case 0:
      return rotate0(src, srcRect);
    case 90:
    case -270:
      return rotate90(src, srcRect);
    case 180:
    case -180:
      return rotate180(src, srcRect);
    case 270:
    case -90:
      return rotate270(src, srcRect);
    default:
      throw std::invalid_argument("orthogonalRotation: invalid angle");
  }
}

BinaryImage orthogonalRotation(const BinaryImage& src, const int degrees) {
  return orthogonalRotation(src, src.rect(), degrees);
}
}  // namespace imageproc