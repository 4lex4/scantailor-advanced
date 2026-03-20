// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "Morphology.h"

#include <QDebug>
#include <cassert>
#include <cmath>
#include <stdexcept>

#include "BinaryImage.h"
#include "GrayImage.h"
#include "Grayscale.h"
#include "RasterOp.h"

namespace imageproc {
Brick::Brick(const QSize& size) {
  const int xOrigin = size.width() >> 1;
  const int yOrigin = size.height() >> 1;
  m_minX = -xOrigin;
  m_minY = -yOrigin;
  m_maxX = (size.width() - 1) - xOrigin;
  m_maxY = (size.height() - 1) - yOrigin;
}

Brick::Brick(const QSize& size, const QPoint& origin) {
  const int xOrigin = origin.x();
  const int yOrigin = origin.y();
  m_minX = -xOrigin;
  m_minY = -yOrigin;
  m_maxX = (size.width() - 1) - xOrigin;
  m_maxY = (size.height() - 1) - yOrigin;
}

Brick::Brick(int minX, int minY, int maxX, int maxY) : m_minX(minX), m_maxX(maxX), m_minY(minY), m_maxY(maxY) {}

void Brick::flip() {
  const int newMinX = -m_maxX;
  m_maxX = -m_minX;
  m_minX = newMinX;
  const int newMinY = -m_maxY;
  m_maxY = -m_minY;
  m_minY = newMinY;
}

Brick Brick::flipped() const {
  Brick brick(*this);
  brick.flip();
  return brick;
}

namespace {
class ReusableImages {
 public:
  void store(BinaryImage& img);

  BinaryImage retrieveOrCreate(const QSize& size);

 private:
  std::vector<BinaryImage> m_images;
};


void ReusableImages::store(BinaryImage& img) {
  assert(!img.isNull());

  // Using push_back(null_image) then swap() avoids atomic operations
  // inside BinaryImage.
  m_images.emplace_back();
  m_images.back().swap(img);
}

BinaryImage ReusableImages::retrieveOrCreate(const QSize& size) {
  if (m_images.empty()) {
    return BinaryImage(size);
  } else {
    BinaryImage img;
    m_images.back().swap(img);
    m_images.pop_back();
    return img;
  }
}

class CoordinateSystem {
 public:
  /**
   * \brief Constructs a global coordinate system.
   */
  CoordinateSystem() : m_origin(0, 0) {}

  /**
   * \brief Constructs a coordinate system relative to the global system.
   */
  explicit CoordinateSystem(const QPoint& origin) : m_origin(origin) {}

  const QPoint& origin() const { return m_origin; }

  QRect fromGlobal(const QRect& rect) const { return rect.translated(-m_origin); }

  QRect toGlobal(const QRect& rect) const { return rect.translated(m_origin); }

  QRect mapTo(const QRect& rect, const CoordinateSystem& targetCs) const {
    return rect.translated(m_origin).translated(-targetCs.origin());
  }

  QPoint offsetTo(const CoordinateSystem& targetCs) const { return m_origin - targetCs.origin(); }

 private:
  QPoint m_origin;
};


void adjustToFit(const QRect& fitInto, QRect& fitAndAdjust, QRect& adjustOnly) {
  int adjLeft = fitInto.left() - fitAndAdjust.left();
  if (adjLeft < 0) {
    adjLeft = 0;
  }

  int adjRight = fitInto.right() - fitAndAdjust.right();
  if (adjRight > 0) {
    adjRight = 0;
  }

  int adjTop = fitInto.top() - fitAndAdjust.top();
  if (adjTop < 0) {
    adjTop = 0;
  }

  int adjBottom = fitInto.bottom() - fitAndAdjust.bottom();
  if (adjBottom > 0) {
    adjBottom = 0;
  }

  fitAndAdjust.adjust(adjLeft, adjTop, adjRight, adjBottom);
  adjustOnly.adjust(adjLeft, adjTop, adjRight, adjBottom);
}

inline QRect extendByBrick(const QRect& rect, const Brick& brick) {
  return rect.adjusted(brick.minX(), brick.minY(), brick.maxX(), brick.maxY());
}

inline QRect shrinkByBrick(const QRect& rect, const Brick& brick) {
  return rect.adjusted(brick.maxX(), brick.maxY(), brick.minX(), brick.minY());
}

const int COMPOSITE_THRESHOLD = 8;

void doInitialCopy(BinaryImage& dst,
                   const CoordinateSystem& dstCs,
                   const QRect& dstRelevantRect,
                   const BinaryImage& src,
                   const CoordinateSystem& srcCs,
                   const BWColor initialColor,
                   const int dx,
                   const int dy) {
  QRect srcRect(src.rect());
  QRect dstRect(srcCs.mapTo(srcRect, dstCs));
  dstRect.translate(dx, dy);
  adjustToFit(dstRelevantRect, dstRect, srcRect);

  rasterOp<RopSrc>(dst, dstRect, src, srcRect.topLeft());

  dst.fillFrame(dstRelevantRect, dstRect, initialColor);
}

void spreadInto(BinaryImage& dst,
                const CoordinateSystem& dstCs,
                const QRect& dstRelevantRect,
                const BinaryImage& src,
                const CoordinateSystem& srcCs,
                const int dxMin,
                const int dxStep,
                const int dyMin,
                const int dyStep,
                const int numSteps,
                const AbstractRasterOp& rop) {
  assert(dxStep == 0 || dyStep == 0);

  int dx = dxMin;
  int dy = dyMin;
  for (int i = 0; i < numSteps; ++i, dx += dxStep, dy += dyStep) {
    QRect srcRect(src.rect());
    QRect dstRect(srcCs.mapTo(srcRect, dstCs));
    dstRect.translate(dx, dy);

    adjustToFit(dstRelevantRect, dstRect, srcRect);

    rop(dst, dstRect, src, srcRect.topLeft());
  }
}

void spreadInDirectionLow(BinaryImage& dst,
                          const CoordinateSystem& dstCs,
                          const QRect& dstRelevantRect,
                          const BinaryImage& src,
                          const CoordinateSystem& srcCs,
                          const int dxMin,
                          const int dxStep,
                          const int dyMin,
                          const int dyStep,
                          const int numSteps,
                          const AbstractRasterOp& rop,
                          const BWColor initialColor,
                          const bool dstCompositionAllowed) {
  assert(dxStep == 0 || dyStep == 0);

  if (numSteps == 0) {
    return;
  }

  doInitialCopy(dst, dstCs, dstRelevantRect, src, srcCs, initialColor, dxMin, dyMin);

  if (numSteps == 1) {
    return;
  }

  int remainingDxMin = dxMin + dxStep;
  int remainingDyMin = dyMin + dyStep;
  int remainingSteps = numSteps - 1;

  if (dstCompositionAllowed) {
    int dx = dxStep;
    int dy = dyStep;
    int i = 1;
    for (; (i << 1) <= numSteps; i <<= 1, dx <<= 1, dy <<= 1) {
      QRect dstRect(dst.rect());
      QRect srcRect(dstRect);
      dstRect.translate(dx, dy);

      adjustToFit(dstRelevantRect, dstRect, srcRect);

      rop(dst, dstRect, dst, srcRect.topLeft());
    }

    remainingDxMin = dxMin + dx;
    remainingDyMin = dyMin + dy;
    remainingSteps = numSteps - i;
  }

  if (remainingSteps > 0) {
    spreadInto(dst, dstCs, dstRelevantRect, src, srcCs, remainingDxMin, dxStep, remainingDyMin, dyStep, remainingSteps,
               rop);
  }
}  // spreadInDirectionLow

void spreadInDirection(BinaryImage& dst,
                       const CoordinateSystem& dstCs,
                       const QRect& dstRelevantRect,
                       const BinaryImage& src,
                       const CoordinateSystem& srcCs,
                       ReusableImages& tmpImages,
                       const CoordinateSystem& tmpCs,
                       const QSize& tmpImageSize,
                       const int dxMin,
                       const int dxStep,
                       const int dyMin,
                       const int dyStep,
                       const int numSteps,
                       const AbstractRasterOp& rop,
                       const BWColor initialColor,
                       const bool dstCompositionAllowed) {
  assert(dxStep == 0 || dyStep == 0);

  if (numSteps < COMPOSITE_THRESHOLD) {
    spreadInDirectionLow(dst, dstCs, dstRelevantRect, src, srcCs, dxMin, dxStep, dyMin, dyStep, numSteps, rop,
                         initialColor, dstCompositionAllowed);
    return;
  }

  const auto firstPhaseSteps = (int) std::sqrt((double) numSteps);

  BinaryImage tmp(tmpImages.retrieveOrCreate(tmpImageSize));

  spreadInDirection(tmp, tmpCs, tmp.rect(), src, srcCs, tmpImages, tmpCs, tmpImageSize, dxMin, dxStep, dyMin, dyStep,
                    firstPhaseSteps, rop, initialColor, true);

  const int secondPhaseSteps = numSteps / firstPhaseSteps;

  spreadInDirection(dst, dstCs, dstRelevantRect, tmp, tmpCs, tmpImages, tmpCs, tmpImageSize, 0,
                    dxStep * firstPhaseSteps, 0, dyStep * firstPhaseSteps, secondPhaseSteps, rop, initialColor,
                    dstCompositionAllowed);

  const int stepsDone = firstPhaseSteps * secondPhaseSteps;
  const int stepsRemaining = numSteps - stepsDone;

  if (stepsRemaining <= 0) {
    assert(stepsRemaining == 0);
  } else if (stepsRemaining < COMPOSITE_THRESHOLD) {
    spreadInto(dst, dstCs, dstRelevantRect, src, srcCs, dxMin + dxStep * stepsDone, dxStep, dyMin + dyStep * stepsDone,
               dyStep, stepsRemaining, rop);
  } else {
    spreadInDirection(tmp, tmpCs, tmp.rect(), src, srcCs, tmpImages, tmpCs, tmpImageSize, dxMin + dxStep * stepsDone,
                      dxStep, dyMin + dyStep * stepsDone, dyStep, stepsRemaining, rop, initialColor, true);

    spreadInto(dst, dstCs, dstRelevantRect, tmp, tmpCs, 0, 0, 0, 0, 1, rop);
  }

  tmpImages.store(tmp);
}  // spreadInDirection

void dilateOrErodeBrick(BinaryImage& dst,
                        const BinaryImage& src,
                        const Brick& brick,
                        const QRect& dstArea,
                        const BWColor srcSurroundings,
                        const AbstractRasterOp& rop,
                        const BWColor spreadingColor) {
  assert(!src.isNull());
  assert(!brick.isEmpty());
  assert(!dstArea.isEmpty());

  if (!extendByBrick(src.rect(), brick).intersects(dstArea)) {
    dst.fill(srcSurroundings);
    return;
  }

  const CoordinateSystem srcCs;  // global coordinate system
  const CoordinateSystem dstCs(dstArea.topLeft());
  const QRect dstImageRect(QPoint(0, 0), dstArea.size());

  // Area in dst coordinates that matters.
  // Everything outside of it will be overwritten.
  QRect dstRelevantRect(dstImageRect);

  if (srcSurroundings == spreadingColor) {
    dstRelevantRect = dstCs.fromGlobal(src.rect());
    dstRelevantRect = shrinkByBrick(dstRelevantRect, brick);
    dstRelevantRect = dstRelevantRect.intersected(dstImageRect);
    if (dstRelevantRect.isEmpty()) {
      dst.fill(srcSurroundings);
      return;
    }
  }

  const QRect tmpArea(
      dstCs.toGlobal(dstRelevantRect).adjusted(-(brick.maxX() - brick.minX()), -(brick.maxY() - brick.minY()), 0, 0));

  CoordinateSystem tmpCs(tmpArea.topLeft());
  const QRect tmpImageRect(QPoint(0, 0), tmpArea.size());

  // Because all temporary images share the same size, it's easy
  // to reuse them.  Reusing an image not only saves us the
  // cost of memory allocation, but also improves chances that
  // image data is already in CPU cache.
  ReusableImages tmpImages;

  if (brick.minY() == brick.maxY()) {
    spreadInDirection(  // horizontal
        dst, dstCs, dstRelevantRect, src, srcCs, tmpImages, tmpCs, tmpImageRect.size(), brick.minX(), 1, brick.minY(),
        0, brick.width(), rop, !spreadingColor, false);
  } else if (brick.minX() == brick.maxX()) {
    spreadInDirection(  // vertical
        dst, dstCs, dstRelevantRect, src, srcCs, tmpImages, tmpCs, tmpImageRect.size(), brick.minX(), 0, brick.minY(),
        1, brick.height(), rop, !spreadingColor, false);
  } else {
    BinaryImage tmp(tmpArea.size());
    spreadInDirection(  // horizontal
        tmp, tmpCs, tmpImageRect, src, srcCs, tmpImages, tmpCs, tmpImageRect.size(), brick.minX(), 1, brick.minY(), 0,
        brick.width(), rop, !spreadingColor, true);

    spreadInDirection(  // vertical
        dst, dstCs, dstRelevantRect, tmp, tmpCs, tmpImages, tmpCs, tmpImageRect.size(), 0, 0, 0, 1, brick.height(), rop,
        !spreadingColor, false);
  }

  if (srcSurroundings == spreadingColor) {
    dst.fillExcept(dstRelevantRect, srcSurroundings);
  }
}  // dilateOrErodeBrick

class Darker {
 public:
  static uint8_t select(uint8_t v1, uint8_t v2) { return std::min(v1, v2); }
};


class Lighter {
 public:
  static uint8_t select(uint8_t v1, uint8_t v2) { return std::max(v1, v2); }
};


template <typename MinOrMax>
void fillExtremumArrayLeftHalf(uint8_t* dst,
                               const uint8_t* const srcCenter,
                               const int srcDelta,
                               const int srcFirstOffset,
                               const int srcCenterOffset) {
  const uint8_t* src = srcCenter;
  uint8_t extremum = *src;
  *dst = extremum;

  for (int i = srcCenterOffset - 1; i >= srcFirstOffset; --i) {
    src -= srcDelta;
    --dst;
    extremum = MinOrMax::select(extremum, *src);
    *dst = extremum;
  }
}

template <typename MinOrMax>
void fillExtremumArrayRightHalf(uint8_t* dst,
                                const uint8_t* const srcCenter,
                                const int srcDelta,
                                const int srcCenterOffset,
                                const int srcLastOffset) {
  const uint8_t* src = srcCenter;
  uint8_t extremum = *src;
  *dst = extremum;

  for (int i = srcCenterOffset + 1; i <= srcLastOffset; ++i) {
    src += srcDelta;
    ++dst;
    extremum = MinOrMax::select(extremum, *src);
    *dst = extremum;
  }
}

template <typename MinOrMax>
void spreadGrayHorizontal(GrayImage& dst, const GrayImage& src, const int dy, const int dx1, const int dx2) {
  const int srcStride = src.stride();
  const int dstStride = dst.stride();
  const uint8_t* srcLine = src.data() + dy * srcStride;
  uint8_t* dstLine = dst.data();

  const int dstWidth = dst.width();
  const int dstHeight = dst.height();

  const int seLen = dx2 - dx1 + 1;

  std::vector<uint8_t> minMaxArray(seLen * 2 - 1, 0);
  uint8_t* const arrayCenter = &minMaxArray[seLen - 1];

  for (int y = 0; y < dstHeight; ++y) {
    for (int dstSegmentFirst = 0; dstSegmentFirst < dstWidth; dstSegmentFirst += seLen) {
      const int dstSegmentLast = std::min(dstSegmentFirst + seLen, dstWidth) - 1;  // inclusive
      const int srcSegmentFirst = dstSegmentFirst + dx1;
      const int srcSegmentLast = dstSegmentLast + dx2;
      const int srcSegmentCenter = (srcSegmentFirst + srcSegmentLast) >> 1;

      fillExtremumArrayLeftHalf<MinOrMax>(arrayCenter, srcLine + srcSegmentCenter, 1, srcSegmentFirst,
                                          srcSegmentCenter);

      fillExtremumArrayRightHalf<MinOrMax>(arrayCenter, srcLine + srcSegmentCenter, 1, srcSegmentCenter,
                                           srcSegmentLast);

      for (int x = dstSegmentFirst; x <= dstSegmentLast; ++x) {
        const int srcFirst = x + dx1;
        const int srcLast = x + dx2;  // inclusive
        assert(srcSegmentCenter >= srcFirst);
        assert(srcSegmentCenter <= srcLast);
        uint8_t v1 = arrayCenter[srcFirst - srcSegmentCenter];
        uint8_t v2 = arrayCenter[srcLast - srcSegmentCenter];
        dstLine[x] = MinOrMax::select(v1, v2);
      }
    }

    srcLine += srcStride;
    dstLine += dstStride;
  }
}  // spreadGrayHorizontal

template <typename MinOrMax>
void spreadGrayHorizontal(GrayImage& dst,
                          const CoordinateSystem& dstCs,
                          const GrayImage& src,
                          const CoordinateSystem& srcCs,
                          const int dy,
                          const int dx1,
                          const int dx2) {
  // src_point = dst_point + dstToSrc;
  const QPoint dstToSrc(dstCs.offsetTo(srcCs));

  spreadGrayHorizontal<MinOrMax>(dst, src, dy + dstToSrc.y(), dx1 + dstToSrc.x(), dx2 + dstToSrc.x());
}

template <typename MinOrMax>
void spreadGrayVertical(GrayImage& dst, const GrayImage& src, const int dx, const int dy1, const int dy2) {
  const int srcStride = src.stride();
  const int dstStride = dst.stride();
  const uint8_t* const srcData = src.data() + dx;
  uint8_t* const dstData = dst.data();

  const int dstWidth = dst.width();
  const int dstHeight = dst.height();

  const int seLen = dy2 - dy1 + 1;

  std::vector<uint8_t> minMaxArray(seLen * 2 - 1, 0);
  uint8_t* const arrayCenter = &minMaxArray[seLen - 1];

  for (int x = 0; x < dstWidth; ++x) {
    for (int dstSegmentFirst = 0; dstSegmentFirst < dstHeight; dstSegmentFirst += seLen) {
      const int dstSegmentLast = std::min(dstSegmentFirst + seLen, dstHeight) - 1;  // inclusive
      const int srcSegmentFirst = dstSegmentFirst + dy1;
      const int srcSegmentLast = dstSegmentLast + dy2;
      const int srcSegmentCenter = (srcSegmentFirst + srcSegmentLast) >> 1;

      fillExtremumArrayLeftHalf<MinOrMax>(arrayCenter, srcData + x + srcSegmentCenter * srcStride, srcStride,
                                          srcSegmentFirst, srcSegmentCenter);

      fillExtremumArrayRightHalf<MinOrMax>(arrayCenter, srcData + x + srcSegmentCenter * srcStride, srcStride,
                                           srcSegmentCenter, srcSegmentLast);

      uint8_t* dst = dstData + x + dstSegmentFirst * dstStride;
      for (int y = dstSegmentFirst; y <= dstSegmentLast; ++y) {
        const int srcFirst = y + dy1;
        const int srcLast = y + dy2;  // inclusive
        assert(srcSegmentCenter >= srcFirst);
        assert(srcSegmentCenter <= srcLast);
        uint8_t v1 = arrayCenter[srcFirst - srcSegmentCenter];
        uint8_t v2 = arrayCenter[srcLast - srcSegmentCenter];
        *dst = MinOrMax::select(v1, v2);
        dst += dstStride;
      }
    }
  }
}  // spreadGrayVertical

template <typename MinOrMax>
void spreadGrayVertical(GrayImage& dst,
                        const CoordinateSystem& dstCs,
                        const GrayImage& src,
                        const CoordinateSystem& srcCs,
                        const int dx,
                        const int dy1,
                        const int dy2) {
  // src_point = dst_point + dstToSrc;
  const QPoint dstToSrc(dstCs.offsetTo(srcCs));

  spreadGrayVertical<MinOrMax>(dst, src, dx + dstToSrc.x(), dy1 + dstToSrc.y(), dy2 + dstToSrc.y());
}

GrayImage extendGrayImage(const GrayImage& src, const QRect& dstArea, const uint8_t background) {
  GrayImage dst(dstArea.size());

  const CoordinateSystem dstCs(dstArea.topLeft());
  const QRect srcRectInDstCs(dstCs.fromGlobal(src.rect()));
  const QRect boundSrcRectInDstCs(srcRectInDstCs.intersected(dst.rect()));

  if (boundSrcRectInDstCs.isEmpty()) {
    dst.fill(background);
    return dst;
  }

  const uint8_t* srcLine = src.data();
  uint8_t* dstLine = dst.data();
  const int srcStride = src.stride();
  const int dstStride = dst.stride();

  int y = 0;
  for (; y < boundSrcRectInDstCs.top(); ++y, dstLine += dstStride) {
    memset(dstLine, background, dstStride);
  }

  const int frontSpanLen = boundSrcRectInDstCs.left();
  const int dataSpanLen = boundSrcRectInDstCs.width();
  const int backSpanOffset = frontSpanLen + dataSpanLen;
  const int backSpanLen = dstArea.width() - backSpanOffset;

  const QPoint srcOffset(boundSrcRectInDstCs.topLeft() - srcRectInDstCs.topLeft());

  srcLine += srcOffset.x() + srcOffset.y() * srcStride;
  for (; y <= boundSrcRectInDstCs.bottom(); ++y) {
    memset(dstLine, background, frontSpanLen);
    memcpy(dstLine + frontSpanLen, srcLine, dataSpanLen);
    memset(dstLine + backSpanOffset, background, backSpanLen);

    srcLine += srcStride;
    dstLine += dstStride;
  }

  const int height = dstArea.height();
  for (; y < height; ++y, dstLine += dstStride) {
    memset(dstLine, background, dstStride);
  }
  return dst;
}  // extendGrayImage

template <typename MinOrMax>
GrayImage dilateOrErodeGray(const GrayImage& src,
                            const Brick& brick,
                            const QRect& dstArea,
                            const unsigned char srcSurroundings) {
  assert(!src.isNull());
  assert(!brick.isEmpty());
  assert(!dstArea.isEmpty());

  GrayImage dst(dstArea.size());

  if (!extendByBrick(src.rect(), brick).intersects(dstArea)) {
    dst.fill(srcSurroundings);
    return dst;
  }
  const CoordinateSystem dstCs(dstArea.topLeft());

  // Each pixel will be a minumum or maximum of a group of pixels
  // in its neighborhood.  The neighborhood is defined by collectArea.
  const Brick collectArea(brick.flipped());

  if ((collectArea.minY() != collectArea.maxY()) && (collectArea.minX() != collectArea.maxX())) {
    // We are going to make two operations:
    // src -> tmp, then tmp -> dst
    // Those operations will use the following collect areas:
    const Brick collectArea1(collectArea.minX(), collectArea.minY(), collectArea.maxX(), collectArea.minY());
    const Brick collectArea2(0, 0, 0, collectArea.maxY() - collectArea.minY());

    const QRect tmpRect(extendByBrick(dstArea, collectArea2));
    CoordinateSystem tmpCs(tmpRect.topLeft());

    GrayImage tmp(tmpRect.size());
    // First operation.  The scope is there to destroy the
    // effectiveSrc image when it's no longer necessary.
    {
      const QRect effectiveSrcRect(extendByBrick(tmpRect, collectArea1));
      GrayImage effectiveSrc;
      CoordinateSystem effectiveSrcCs;
      if (src.rect().contains(effectiveSrcRect)) {
        effectiveSrc = src;
      } else {
        effectiveSrc = extendGrayImage(src, effectiveSrcRect, srcSurroundings);
        effectiveSrcCs = CoordinateSystem(effectiveSrcRect.topLeft());
      }

      spreadGrayHorizontal<MinOrMax>(tmp, tmpCs, effectiveSrc, effectiveSrcCs, collectArea1.minY(), collectArea1.minX(),
                                     collectArea1.maxX());
    }
    // Second operation.
    spreadGrayVertical<MinOrMax>(dst, dstCs, tmp, tmpCs, collectArea2.minX(), collectArea2.minY(), collectArea2.maxY());
  } else {
    const QRect effectiveSrcRect(extendByBrick(dstArea, collectArea));
    GrayImage effectiveSrc;
    CoordinateSystem effectiveSrcCs;
    if (src.rect().contains(effectiveSrcRect)) {
      effectiveSrc = src;
    } else {
      effectiveSrc = extendGrayImage(src, effectiveSrcRect, srcSurroundings);
      effectiveSrcCs = CoordinateSystem(effectiveSrcRect.topLeft());
    }

    if (collectArea.minY() == collectArea.maxY()) {
      spreadGrayHorizontal<MinOrMax>(dst, dstCs, effectiveSrc, effectiveSrcCs, collectArea.minY(), collectArea.minX(),
                                     collectArea.maxX());
    } else {
      assert(collectArea.minX() == collectArea.maxX());
      spreadGrayVertical<MinOrMax>(dst, dstCs, effectiveSrc, effectiveSrcCs, collectArea.minX(), collectArea.minY(),
                                   collectArea.maxY());
    }
  }
  return dst;
}  // dilateOrErodeGray
}  // anonymous namespace

BinaryImage dilateBrick(const BinaryImage& src,
                        const Brick& brick,
                        const QRect& dstArea,
                        const BWColor srcSurroundings) {
  if (src.isNull()) {
    throw std::invalid_argument("dilateBrick: src image is null");
  }
  if (brick.isEmpty()) {
    throw std::invalid_argument("dilateBrick: brick is empty");
  }
  if (dstArea.isEmpty()) {
    throw std::invalid_argument("dilateBrick: dstArea is empty");
  }

  TemplateRasterOp<RopOr<RopSrc, RopDst>> rop;
  BinaryImage dst(dstArea.size());
  dilateOrErodeBrick(dst, src, brick, dstArea, srcSurroundings, rop, BLACK);
  return dst;
}

GrayImage dilateGray(const GrayImage& src,
                     const Brick& brick,
                     const QRect& dstArea,
                     const unsigned char srcSurroundings) {
  if (src.isNull()) {
    throw std::invalid_argument("dilateGray: src image is null");
  }
  if (brick.isEmpty()) {
    throw std::invalid_argument("dilateGray: brick is empty");
  }
  if (dstArea.isEmpty()) {
    throw std::invalid_argument("dilateGray: dstArea is empty");
  }
  return dilateOrErodeGray<Darker>(src, brick, dstArea, srcSurroundings);
}

BinaryImage dilateBrick(const BinaryImage& src, const Brick& brick, const BWColor srcSurroundings) {
  return dilateBrick(src, brick, src.rect(), srcSurroundings);
}

GrayImage dilateGray(const GrayImage& src, const Brick& brick, const unsigned char srcSurroundings) {
  return dilateGray(src, brick, src.rect(), srcSurroundings);
}

BinaryImage erodeBrick(const BinaryImage& src,
                       const Brick& brick,
                       const QRect& dstArea,
                       const BWColor srcSurroundings) {
  if (src.isNull()) {
    throw std::invalid_argument("erodeBrick: src image is null");
  }
  if (brick.isEmpty()) {
    throw std::invalid_argument("erodeBrick: brick is empty");
  }
  if (dstArea.isEmpty()) {
    throw std::invalid_argument("erodeBrick: dstArea is empty");
  }

  using Rop = RopAnd<RopSrc, RopDst>;

  TemplateRasterOp<RopAnd<RopSrc, RopDst>> rop;
  BinaryImage dst(dstArea.size());
  dilateOrErodeBrick(dst, src, brick, dstArea, srcSurroundings, rop, WHITE);
  return dst;
}

GrayImage erodeGray(const GrayImage& src,
                    const Brick& brick,
                    const QRect& dstArea,
                    const unsigned char srcSurroundings) {
  if (src.isNull()) {
    throw std::invalid_argument("erodeGray: src image is null");
  }
  if (brick.isEmpty()) {
    throw std::invalid_argument("erodeGray: brick is empty");
  }
  if (dstArea.isEmpty()) {
    throw std::invalid_argument("erodeGray: dstArea is empty");
  }
  return dilateOrErodeGray<Lighter>(src, brick, dstArea, srcSurroundings);
}

BinaryImage erodeBrick(const BinaryImage& src, const Brick& brick, const BWColor srcSurroundings) {
  return erodeBrick(src, brick, src.rect(), srcSurroundings);
}

GrayImage erodeGray(const GrayImage& src, const Brick& brick, const unsigned char srcSurroundings) {
  return erodeGray(src, brick, src.rect(), srcSurroundings);
}

BinaryImage openBrick(const BinaryImage& src, const QSize& brick, const QRect& dstArea, const BWColor srcSurroundings) {
  if (src.isNull()) {
    throw std::invalid_argument("openBrick: src image is null");
  }
  if (brick.isEmpty()) {
    throw std::invalid_argument("openBrick: brick is empty");
  }

  Brick actualBrick(brick);

  QRect tmpArea;

  if (srcSurroundings == WHITE) {
    tmpArea = shrinkByBrick(src.rect(), actualBrick);
    if (tmpArea.isEmpty()) {
      return BinaryImage(dstArea.size(), WHITE);
    }
  } else {
    tmpArea = extendByBrick(src.rect(), actualBrick);
  }

  // At this point we could leave tmpArea as is, but a large
  // tmpArea would be a waste if dstArea is small.

  tmpArea = extendByBrick(dstArea, actualBrick).intersected(tmpArea);

  CoordinateSystem tmpCs(tmpArea.topLeft());

  const BinaryImage tmp(erodeBrick(src, actualBrick, tmpArea, srcSurroundings));
  actualBrick.flip();
  return dilateBrick(tmp, actualBrick, tmpCs.fromGlobal(dstArea), srcSurroundings);
}  // openBrick

BinaryImage openBrick(const BinaryImage& src, const QSize& brick, const BWColor srcSurroundings) {
  return openBrick(src, brick, src.rect(), srcSurroundings);
}

GrayImage openGray(const GrayImage& src,
                   const QSize& brick,
                   const QRect& dstArea,
                   const unsigned char srcSurroundings) {
  if (src.isNull()) {
    throw std::invalid_argument("openGray: src image is null");
  }
  if (brick.isEmpty()) {
    throw std::invalid_argument("openGray: brick is empty");
  }
  if (dstArea.isEmpty()) {
    throw std::invalid_argument("openGray: dstArea is empty");
  }

  const Brick brick1(brick);
  const Brick brick2(brick1.flipped());

  // We are going to make two operations:
  // tmp = erodeGray(src, brick1), then dst = dilateGray(tmp, brick2)
  const QRect tmpRect(extendByBrick(dstArea, brick1));
  CoordinateSystem tmpCs(tmpRect.topLeft());

  const GrayImage tmp(dilateOrErodeGray<Lighter>(src, brick1, tmpRect, srcSurroundings));
  return dilateOrErodeGray<Darker>(tmp, brick2, tmpCs.fromGlobal(dstArea), srcSurroundings);
}

GrayImage openGray(const GrayImage& src, const QSize& brick, const unsigned char srcSurroundings) {
  return openGray(src, brick, src.rect(), srcSurroundings);
}

BinaryImage closeBrick(const BinaryImage& src,
                       const QSize& brick,
                       const QRect& dstArea,
                       const BWColor srcSurroundings) {
  if (src.isNull()) {
    throw std::invalid_argument("closeBrick: src image is null");
  }
  if (brick.isEmpty()) {
    throw std::invalid_argument("closeBrick: brick is empty");
  }

  Brick actualBrick(brick);

  QRect tmpArea;

  if (srcSurroundings == BLACK) {
    tmpArea = shrinkByBrick(src.rect(), actualBrick);
    if (tmpArea.isEmpty()) {
      return BinaryImage(dstArea.size(), BLACK);
    }
  } else {
    tmpArea = extendByBrick(src.rect(), actualBrick);
  }

  // At this point we could leave tmpArea as is, but a large
  // tmpArea would be a waste if dstArea is small.

  tmpArea = extendByBrick(dstArea, actualBrick).intersected(tmpArea);

  CoordinateSystem tmpCs(tmpArea.topLeft());

  const BinaryImage tmp(dilateBrick(src, actualBrick, tmpArea, srcSurroundings));
  actualBrick.flip();
  return erodeBrick(tmp, actualBrick, tmpCs.fromGlobal(dstArea), srcSurroundings);
}  // closeBrick

BinaryImage closeBrick(const BinaryImage& src, const QSize& brick, const BWColor srcSurroundings) {
  return closeBrick(src, brick, src.rect(), srcSurroundings);
}

GrayImage closeGray(const GrayImage& src,
                    const QSize& brick,
                    const QRect& dstArea,
                    const unsigned char srcSurroundings) {
  if (src.isNull()) {
    throw std::invalid_argument("closeGray: src image is null");
  }
  if (brick.isEmpty()) {
    throw std::invalid_argument("closeGray: brick is empty");
  }
  if (dstArea.isEmpty()) {
    throw std::invalid_argument("closeGray: dstArea is empty");
  }

  const Brick brick1(brick);
  const Brick brick2(brick1.flipped());

  // We are going to make two operations:
  // tmp = dilateGray(src, brick1), then dst = erodeGray(tmp, brick2)
  const QRect tmpRect(extendByBrick(dstArea, brick2));
  CoordinateSystem tmpCs(tmpRect.topLeft());

  const GrayImage tmp(dilateOrErodeGray<Darker>(src, brick1, tmpRect, srcSurroundings));
  return dilateOrErodeGray<Lighter>(tmp, brick2, tmpCs.fromGlobal(dstArea), srcSurroundings);
}

GrayImage closeGray(const GrayImage& src, const QSize& brick, const unsigned char srcSurroundings) {
  return closeGray(src, brick, src.rect(), srcSurroundings);
}

BinaryImage hitMissMatch(const BinaryImage& src,
                         const BWColor srcSurroundings,
                         const std::vector<QPoint>& hits,
                         const std::vector<QPoint>& misses) {
  if (src.isNull()) {
    return BinaryImage();
  }

  const QRect rect(src.rect());  // same as dst.rect()
  BinaryImage dst(src.size());

  bool first = true;

  for (const QPoint& hit : hits) {
    QRect srcRect(rect);
    QRect dstRect(rect.translated(-hit));
    adjustToFit(rect, dstRect, srcRect);

    if (first) {
      first = false;
      rasterOp<RopSrc>(dst, dstRect, src, srcRect.topLeft());
      if (srcSurroundings == BLACK) {
        dst.fillExcept(dstRect, BLACK);
      }
    } else {
      rasterOp<RopAnd<RopSrc, RopDst>>(dst, dstRect, src, srcRect.topLeft());
    }

    if (srcSurroundings == WHITE) {
      // No hits on white surroundings.
      dst.fillExcept(dstRect, WHITE);
    }
  }

  for (const QPoint& miss : misses) {
    QRect srcRect(rect);
    QRect dstRect(rect.translated(-miss));
    adjustToFit(rect, dstRect, srcRect);

    if (first) {
      first = false;
      rasterOp<RopNot<RopSrc>>(dst, dstRect, src, srcRect.topLeft());
      if (srcSurroundings == WHITE) {
        dst.fillExcept(dstRect, BLACK);
      }
    } else {
      rasterOp<RopAnd<RopNot<RopSrc>, RopDst>>(dst, dstRect, src, srcRect.topLeft());
    }

    if (srcSurroundings == BLACK) {
      // No misses on black surroundings.
      dst.fillExcept(dstRect, WHITE);
    }
  }

  if (first) {
    dst.fill(WHITE);  // No matches.
  }
  return dst;
}  // hitMissMatch

BinaryImage hitMissMatch(const BinaryImage& src,
                         const BWColor srcSurroundings,
                         const char* const pattern,
                         const int patternWidth,
                         const int patternHeight,
                         const QPoint& patternOrigin) {
  std::vector<QPoint> hits;
  std::vector<QPoint> misses;

  const char* p = pattern;
  for (int y = 0; y < patternHeight; ++y) {
    for (int x = 0; x < patternWidth; ++x, ++p) {
      switch (*p) {
        case 'X':
          hits.push_back(QPoint(x, y) - patternOrigin);
          break;
        case ' ':
          misses.push_back(QPoint(x, y) - patternOrigin);
          break;
        case '?':
          break;
        default:
          throw std::invalid_argument("hitMissMatch: invalid character in pattern");
      }
    }
  }
  return hitMissMatch(src, srcSurroundings, hits, misses);
}

BinaryImage hitMissReplace(const BinaryImage& src,
                           const BWColor srcSurroundings,
                           const char* const pattern,
                           const int patternWidth,
                           const int patternHeight) {
  BinaryImage dst(src);

  hitMissReplaceInPlace(dst, srcSurroundings, pattern, patternWidth, patternHeight);
  return dst;
}

void hitMissReplaceInPlace(BinaryImage& img,
                           const BWColor srcSurroundings,
                           const char* const pattern,
                           const int patternWidth,
                           const int patternHeight) {
  // It's better to have the origin at one of the replacement positions.
  // Otherwise we may miss a partially outside-of-image match because
  // the origin point was outside of the image as well.
  const int patternLen = patternWidth * patternHeight;
  const auto* const minusPos = (const char*) memchr(pattern, '-', patternLen);
  const auto* const plusPos = (const char*) memchr(pattern, '+', patternLen);
  const char* originPos;
  if (minusPos && plusPos) {
    originPos = std::min(minusPos, plusPos);
  } else if (minusPos) {
    originPos = minusPos;
  } else if (plusPos) {
    originPos = plusPos;
  } else {
    // No replacements requested - nothing to do.
    return;
  }

  const QPoint origin(static_cast<int>((originPos - pattern) % patternWidth),
                      static_cast<int>((originPos - pattern) / patternWidth));

  std::vector<QPoint> hits;
  std::vector<QPoint> misses;
  std::vector<QPoint> whiteToBlack;
  std::vector<QPoint> blackToWhite;

  const char* p = pattern;
  for (int y = 0; y < patternHeight; ++y) {
    for (int x = 0; x < patternWidth; ++x, ++p) {
      switch (*p) {
        case '-':
          blackToWhite.push_back(QPoint(x, y) - origin);
          // fall through
        case 'X':
          hits.push_back(QPoint(x, y) - origin);
          break;
        case '+':
          whiteToBlack.push_back(QPoint(x, y) - origin);
          // fall through
        case ' ':
          misses.push_back(QPoint(x, y) - origin);
          break;
        case '?':
          break;
        default:
          throw std::invalid_argument("hitMissReplace: invalid character in pattern");
      }
    }
  }

  const BinaryImage matches(hitMissMatch(img, srcSurroundings, hits, misses));
  const QRect rect(img.rect());

  for (const QPoint& offset : whiteToBlack) {
    QRect srcRect(rect);
    QRect dstRect(rect.translated(offset));
    adjustToFit(rect, dstRect, srcRect);

    rasterOp<RopOr<RopSrc, RopDst>>(img, dstRect, matches, srcRect.topLeft());
  }

  for (const QPoint& offset : blackToWhite) {
    QRect srcRect(rect);
    QRect dstRect(rect.translated(offset));
    adjustToFit(rect, dstRect, srcRect);

    rasterOp<RopSubtract<RopDst, RopSrc>>(img, dstRect, matches, srcRect.topLeft());
  }
}

BinaryImage whiteTopHatTransform(const BinaryImage& src,
                                 const QSize& brick,
                                 const QRect& dstArea,
                                 BWColor srcSurroundings) {
  if (src.isNull()) {
    throw std::invalid_argument("whiteTopHatTransform: src image is null");
  }
  if (brick.isEmpty()) {
    throw std::invalid_argument("whiteTopHatTransform: brick is empty");
  }
  if (dstArea.isEmpty()) {
    throw std::invalid_argument("whiteTopHatTransform: dstArea is empty");
  }

  BinaryImage dst(dstArea.size());
  if (dstArea != src.rect()) {
    rasterOp<RopSrc>(dst, dst.rect(), src, dstArea.topLeft());
  } else {
    dst = src;
  }

  rasterOp<RopSubtract<RopDst, RopSrc>>(dst, openBrick(src, brick, dstArea, srcSurroundings));
  return dst;
}

BinaryImage whiteTopHatTransform(const BinaryImage& src, const QSize& brick, BWColor srcSurroundings) {
  return whiteTopHatTransform(src, brick, src.rect(), srcSurroundings);
}

BinaryImage blackTopHatTransform(const BinaryImage& src,
                                 const QSize& brick,
                                 const QRect& dstArea,
                                 BWColor srcSurroundings) {
  if (src.isNull()) {
    throw std::invalid_argument("blackTopHatTransform: src image is null");
  }
  if (brick.isEmpty()) {
    throw std::invalid_argument("blackTopHatTransform: brick is empty");
  }
  if (dstArea.isEmpty()) {
    throw std::invalid_argument("blackTopHatTransform: dstArea is empty");
  }

  BinaryImage dst(dstArea.size());
  if (dstArea != src.rect()) {
    rasterOp<RopSrc>(dst, dst.rect(), src, dstArea.topLeft());
  } else {
    dst = src;
  }

  rasterOp<RopSubtract<RopSrc, RopDst>>(dst, closeBrick(src, brick, dstArea, srcSurroundings));
  return dst;
}

BinaryImage blackTopHatTransform(const BinaryImage& src, const QSize& brick, BWColor srcSurroundings) {
  return blackTopHatTransform(src, brick, src.rect(), srcSurroundings);
}
}  // namespace imageproc
