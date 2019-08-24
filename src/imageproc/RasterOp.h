// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_IMAGEPROC_RASTEROP_H_
#define SCANTAILOR_IMAGEPROC_RASTEROP_H_

#include <QPoint>
#include <QRect>
#include <QSize>
#include <boost/cstdint.hpp>
#include <cassert>
#include <stdexcept>
#include "BinaryImage.h"

namespace imageproc {
/**
 * \brief Perform pixel-wise logical operations on portions of images.
 *
 * \param dst The destination image.  Changes will be written there.
 * \param dr The rectangle within the destination image to process.
 * \param src The source image.  May be the same as the destination image.
 * \param sp The top-left corner of the rectangle within the source image
 *           to process.  The rectangle itself is assumed to be the same
 *           as the destination rectangle.
 *
 * The template argument is the operation to perform.  This is generally
 * a combination of several Rop* class templates, such as RopXor\<RopSrc, RopDst\>.
 */
template <typename Rop>
void rasterOp(BinaryImage& dst, const QRect& dr, const BinaryImage& src, const QPoint& sp);

/**
 * \brief Perform pixel-wise logical operations on whole images.
 *
 * \param dst The destination image.  Changes will be written there.
 * \param src The source image.  May be the same as the destination image,
 *        otherwise it must have the same dimensions.
 *
 * The template argument is the operation to perform.  This is generally
 * a combination of several Rop* class templates, such as RopXor\<RopSrc, RopDst\>.
 */
template <typename Rop>
void rasterOp(BinaryImage& dst, const BinaryImage& src);

/**
 * \brief Raster operation that takes source pixels as they are.
 * \see rasterOp()
 */
class RopSrc {
 public:
  static uint32_t transform(uint32_t src, uint32_t /*dst*/) { return src; }
};


/**
 * \brief Raster operation that takes destination pixels as they are.
 * \see rasterOp()
 */
class RopDst {
 public:
  static uint32_t transform(uint32_t /*src*/, uint32_t dst) { return dst; }
};


/**
 * \brief Raster operation that performs a logical NOT operation.
 * \see rasterOp()
 */
template <typename Arg>
class RopNot {
 public:
  static uint32_t transform(uint32_t src, uint32_t dst) { return ~Arg::transform(src, dst); }
};


/**
 * \brief Raster operation that performs a logical AND operation.
 * \see rasterOp()
 */
template <typename Arg1, typename Arg2>
class RopAnd {
 public:
  static uint32_t transform(uint32_t src, uint32_t dst) {
    return Arg1::transform(src, dst) & Arg2::transform(src, dst);
  }
};


/**
 * \brief Raster operation that performs a logical OR operation.
 * \see rasterOp()
 */
template <typename Arg1, typename Arg2>
class RopOr {
 public:
  static uint32_t transform(uint32_t src, uint32_t dst) {
    return Arg1::transform(src, dst) | Arg2::transform(src, dst);
  }
};


/**
 * \brief Raster operation that performs a logical XOR operation.
 * \see rasterOp()
 */
template <typename Arg1, typename Arg2>
class RopXor {
 public:
  static uint32_t transform(uint32_t src, uint32_t dst) {
    return Arg1::transform(src, dst) ^ Arg2::transform(src, dst);
  }
};


/**
 * \brief Raster operation that subtracts black pixels of Arg2 from Arg1.
 * \see rasterOp()
 */
template <typename Arg1, typename Arg2>
class RopSubtract {
 public:
  static uint32_t transform(uint32_t src, uint32_t dst) {
    uint32_t lhs = Arg1::transform(src, dst);
    uint32_t rhs = Arg2::transform(src, dst);

    return lhs & (lhs ^ rhs);
  }
};


/**
 * \brief Raster operation that subtracts white pixels of Arg2 from Arg1.
 * \see rasterOp()
 */
template <typename Arg1, typename Arg2>
class RopSubtractWhite {
 public:
  static uint32_t transform(uint32_t src, uint32_t dst) {
    uint32_t lhs = Arg1::transform(src, dst);
    uint32_t rhs = Arg2::transform(src, dst);

    return lhs | ~(lhs ^ rhs);
  }
};


/**
 * \brief Polymorphic interface for raster operations.
 *
 * If you want to parametrize some of your code with a raster operation,
 * one way to do it is to have Rop as a template argument.  The other,
 * and usually better way is to have this class as a non-template argument.
 */
class AbstractRasterOp {
 public:
  virtual ~AbstractRasterOp() = default;

  /**
   * \see rasterOp()
   */
  virtual void operator()(BinaryImage& dst, const QRect& dr, const BinaryImage& src, const QPoint& sp) const = 0;
};


/**
 * \brief A pre-defined raster operation to be called polymorphically.
 */
template <typename Rop>
class TemplateRasterOp : public AbstractRasterOp {
 public:
  /**
   * \see rasterOp()
   */
  void operator()(BinaryImage& dst, const QRect& dr, const BinaryImage& src, const QPoint& sp) const override {
    rasterOp<Rop>(dst, dr, src, sp);
  }
};


namespace detail {
template <typename Rop>
void rasterOpInDirection(BinaryImage& dst,
                         const QRect& dr,
                         const BinaryImage& src,
                         const QPoint& sp,
                         const int dy,
                         const int dx) {
  const int srcStartBit = sp.x() % 32;
  const int dstStartBit = dr.x() % 32;
  const int rightmostDstBit = dr.right();  // == dr.x() + dr.width() - 1;
  const int rightmostDstWord = rightmostDstBit / 32 - dr.x() / 32;
  const uint32_t leftmostDstMask = ~uint32_t(0) >> dstStartBit;
  const uint32_t rightmostDstMask = ~uint32_t(0) << (31 - rightmostDstBit % 32);

  int firstDstWord;
  int lastDstWord;
  uint32_t firstDstMask;
  uint32_t lastDstMask;
  if (dx == 1) {
    firstDstWord = 0;
    lastDstWord = rightmostDstWord;
    firstDstMask = leftmostDstMask;
    lastDstMask = rightmostDstMask;
  } else {
    assert(dx == -1);
    firstDstWord = rightmostDstWord;
    lastDstWord = 0;
    firstDstMask = rightmostDstMask;
    lastDstMask = leftmostDstMask;
  }

  int srcSpanDelta;
  int dstSpanDelta;
  uint32_t* dstSpan;
  const uint32_t* srcSpan;
  if (dy == 1) {
    srcSpanDelta = src.wordsPerLine();
    dstSpanDelta = dst.wordsPerLine();
    dstSpan = dst.data() + dr.y() * dstSpanDelta + dr.x() / 32;
    srcSpan = src.data() + sp.y() * srcSpanDelta + sp.x() / 32;
  } else {
    assert(dy == -1);
    srcSpanDelta = -src.wordsPerLine();
    dstSpanDelta = -dst.wordsPerLine();
    assert(dr.bottom() == dr.y() + dr.height() - 1);
    dstSpan = dst.data() - dr.bottom() * dstSpanDelta + dr.x() / 32;
    srcSpan = src.data() - (sp.y() + dr.height() - 1) * srcSpanDelta + sp.x() / 32;
  }

  int srcWord1Shift;
  int srcWord2Shift;
  if (srcStartBit > dstStartBit) {
    srcWord1Shift = srcStartBit - dstStartBit;
    srcWord2Shift = 32 - srcWord1Shift;
  } else if (srcStartBit < dstStartBit) {
    srcWord2Shift = dstStartBit - srcStartBit;
    srcWord1Shift = 32 - srcWord2Shift;
    --srcSpan;
  } else {
    // Here we have a simple case of dst_x % 32 == src_x % 32.
    // Note that the rest of the code doesn't work with such
    // a case because of hardcoded widx + 1.
    if (firstDstWord == lastDstWord) {
      assert(firstDstWord == 0);
      const uint32_t mask = firstDstMask & lastDstMask;

      for (int i = dr.height(); i > 0; --i, srcSpan += srcSpanDelta, dstSpan += dstSpanDelta) {
        const uint32_t srcWord = srcSpan[0];
        const uint32_t dstWord = dstSpan[0];
        const uint32_t newDstWord = Rop::transform(srcWord, dstWord);
        dstSpan[0] = (dstWord & ~mask) | (newDstWord & mask);
      }
    } else {
      for (int i = dr.height(); i > 0; --i, srcSpan += srcSpanDelta, dstSpan += dstSpanDelta) {
        int widx = firstDstWord;
        // Handle the first (possibly incomplete) dst word in the line.
        uint32_t srcWord = srcSpan[widx];
        uint32_t dstWord = dstSpan[widx];
        uint32_t newDstWord = Rop::transform(srcWord, dstWord);
        dstSpan[widx] = (dstWord & ~firstDstMask) | (newDstWord & firstDstMask);

        while ((widx += dx) != lastDstWord) {
          srcWord = srcSpan[widx];
          dstWord = dstSpan[widx];
          dstSpan[widx] = Rop::transform(srcWord, dstWord);
        }

        // Handle the last (possibly incomplete) dst word in the line.
        srcWord = srcSpan[widx];
        dstWord = dstSpan[widx];
        newDstWord = Rop::transform(srcWord, dstWord);
        dstSpan[widx] = (dstWord & ~lastDstMask) | (newDstWord & lastDstMask);
      }
    }

    return;
  }

  if (firstDstWord == lastDstWord) {
    assert(firstDstWord == 0);
    const uint32_t mask = firstDstMask & lastDstMask;
    const uint32_t canWord1 = (~uint32_t(0) << srcWord1Shift) & mask;
    const uint32_t canWord2 = (~uint32_t(0) >> srcWord2Shift) & mask;

    for (int i = dr.height(); i > 0; --i, srcSpan += srcSpanDelta, dstSpan += dstSpanDelta) {
      uint32_t srcWord = 0;
      if (canWord1) {
        const uint32_t srcWord1 = srcSpan[0];
        srcWord |= srcWord1 << srcWord1Shift;
      }
      if (canWord2) {
        const uint32_t srcWord2 = srcSpan[1];
        srcWord |= srcWord2 >> srcWord2Shift;
      }
      const uint32_t dstWord = dstSpan[0];
      const uint32_t newDstWord = Rop::transform(srcWord, dstWord);
      dstSpan[0] = (dstWord & ~mask) | (newDstWord & mask);
    }
  } else {
    const uint32_t canFirstWord1 = (~uint32_t(0) << srcWord1Shift) & firstDstMask;
    const uint32_t canFirstWord2 = (~uint32_t(0) >> srcWord2Shift) & firstDstMask;
    const uint32_t canLastWord1 = (~uint32_t(0) << srcWord1Shift) & lastDstMask;
    const uint32_t canLastWord2 = (~uint32_t(0) >> srcWord2Shift) & lastDstMask;

    for (int i = dr.height(); i > 0; --i, srcSpan += srcSpanDelta, dstSpan += dstSpanDelta) {
      int widx = firstDstWord;
      // Handle the first (possibly incomplete) dst word in the line.
      uint32_t srcWord = 0;
      if (canFirstWord1) {
        const uint32_t srcWord1 = srcSpan[widx];
        srcWord |= srcWord1 << srcWord1Shift;
      }
      if (canFirstWord2) {
        const uint32_t srcWord2 = srcSpan[widx + 1];
        srcWord |= srcWord2 >> srcWord2Shift;
      }
      uint32_t dstWord = dstSpan[widx];
      uint32_t newDstWord = Rop::transform(srcWord, dstWord);
      newDstWord = (dstWord & ~firstDstMask) | (newDstWord & firstDstMask);

      while ((widx += dx) != lastDstWord) {
        const uint32_t srcWord1 = srcSpan[widx];
        const uint32_t srcWord2 = srcSpan[widx + 1];

        dstWord = dstSpan[widx];
        dstSpan[widx - dx] = newDstWord;

        newDstWord = Rop::transform((srcWord1 << srcWord1Shift) | (srcWord2 >> srcWord2Shift), dstWord);
      }

      // Handle the last (possibly incomplete) dst word in the line.
      srcWord = 0;
      if (canLastWord1) {
        const uint32_t srcWord1 = srcSpan[widx];
        srcWord |= srcWord1 << srcWord1Shift;
      }
      if (canLastWord2) {
        const uint32_t srcWord2 = srcSpan[widx + 1];
        srcWord |= srcWord2 >> srcWord2Shift;
      }

      dstWord = dstSpan[widx];
      dstSpan[widx - dx] = newDstWord;

      newDstWord = Rop::transform(srcWord, dstWord);
      newDstWord = (dstWord & ~lastDstMask) | (newDstWord & lastDstMask);
      dstSpan[widx] = newDstWord;
    }
  }
}  // rasterOpInDirection
}  // namespace detail

template <typename Rop>
void rasterOp(BinaryImage& dst, const QRect& dr, const BinaryImage& src, const QPoint& sp) {
  using namespace detail;

  if (dr.isEmpty()) {
    return;
  }

  if (dst.isNull() || src.isNull()) {
    throw std::invalid_argument("rasterOp: can't operate on null images");
  }

  if (!dst.rect().contains(dr)) {
    throw std::invalid_argument("rasterOp: raster area exceedes the dst image");
  }

  if (!src.rect().contains(QRect(sp, dr.size()))) {
    throw std::invalid_argument("rasterOp: raster area exceedes the src image");
  }

  // We need to avoid a situation where we write some output
  // and then read it as input.  This can happen if src and dst
  // are the same images.

  if (&dst == &src) {
    // Note that if src and dst are different objects sharing
    // the same data, dst will get a private copy when
    // dst.data() is called.

    if (dr.y() > sp.y()) {
      rasterOpInDirection<Rop>(dst, dr, src, sp, -1, 1);

      return;
    }

    if ((dr.y() == sp.y()) && (dr.x() > sp.x())) {
      rasterOpInDirection<Rop>(dst, dr, src, sp, 1, -1);

      return;
    }
  }

  rasterOpInDirection<Rop>(dst, dr, src, sp, 1, 1);
}  // rasterOp

template <typename Rop>
void rasterOp(BinaryImage& dst, const BinaryImage& src) {
  using namespace detail;

  if (dst.isNull() || src.isNull()) {
    throw std::invalid_argument("rasterOp: can't operate on null images");
  }

  if (dst.size() != src.size()) {
    throw std::invalid_argument("rasterOp: images have different sizes");
  }

  rasterOpInDirection<Rop>(dst, dst.rect(), src, QPoint(0, 0), 1, 1);
}
}  // namespace imageproc
#endif  // ifndef SCANTAILOR_IMAGEPROC_RASTEROP_H_
