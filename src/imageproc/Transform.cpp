// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

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
  QPolygonF dstPoly;
  dstPoly.push_back(QPointF(0.5, 0.0));
  dstPoly.push_back(QPointF(1.0, 0.5));
  dstPoly.push_back(QPointF(0.5, 1.0));
  dstPoly.push_back(QPointF(0.0, 0.5));

  QPolygonF srcPoly(xform.map(dstPoly));
  std::sort(srcPoly.begin(), srcPoly.end(), XLess());
  const double width = srcPoly.back().x() - srcPoly.front().x();
  std::sort(srcPoly.begin(), srcPoly.end(), YLess());
  const double height = srcPoly.back().y() - srcPoly.front().y();

  const QSizeF min32(min * 32.0);
  return QSizeF(std::max(min32.width(), width), std::max(min32.height(), height));
}

template <typename StorageUnit, typename Mixer>
static void transformGeneric(const StorageUnit* const srcData,
                             const int srcStride,
                             const QSize srcSize,
                             StorageUnit* const dstData,
                             const int dstStride,
                             const QTransform& xform,
                             const QRect& dstRect,
                             const StorageUnit outsideColor,
                             const int outsideFlags,
                             const QSizeF& minMappingArea) {
  const int sw = srcSize.width();
  const int sh = srcSize.height();
  const int dw = dstRect.width();
  const int dh = dstRect.height();

  StorageUnit* dstLine = dstData;

  QTransform invXform;
  invXform.translate(dstRect.x(), dstRect.y());
  invXform *= xform.inverted();
  invXform *= QTransform().scale(32.0, 32.0);

  // sx32 = dx*invXform.m11() + dy*invXform.m21() + invXform.dx();
  // sy32 = dy*invXform.m22() + dx*invXform.m12() + invXform.dy();

  const QSizeF src32UnitSize(calcSrcUnitSize(invXform, minMappingArea));
  const int src32UnitW = std::max<int>(1, qRound(src32UnitSize.width()));
  const int src32UnitH = std::max<int>(1, qRound(src32UnitSize.height()));

  for (int dy = 0; dy < dh; ++dy, dstLine += dstStride) {
    const double fDyCenter = dy + 0.5;
    const double fSx32Base = fDyCenter * invXform.m21() + invXform.dx();
    const double fSy32Base = fDyCenter * invXform.m22() + invXform.dy();

    for (int dx = 0; dx < dw; ++dx) {
      const double fDxCenter = dx + 0.5;
      const double fSx32Center = fSx32Base + fDxCenter * invXform.m11();
      const double fSy32Center = fSy32Base + fDxCenter * invXform.m12();
      int src32Left = (int) fSx32Center - (src32UnitW >> 1);
      int src32Top = (int) fSy32Center - (src32UnitH >> 1);
      int src32Right = src32Left + src32UnitW;
      int src32Bottom = src32Top + src32UnitH;
      int srcLeft = src32Left >> 5;
      int srcRight = (src32Right - 1) >> 5;  // inclusive
      int srcTop = src32Top >> 5;
      int srcBottom = (src32Bottom - 1) >> 5;  // inclusive
      assert(srcBottom >= srcTop);
      assert(srcRight >= srcLeft);

      if ((srcBottom < 0) || (srcRight < 0) || (srcLeft >= sw) || (srcTop >= sh)) {
        // Completely outside of src image.
        if (outsideFlags & OutsidePixels::COLOR) {
          dstLine[dx] = outsideColor;
        } else {
          const int srcX = qBound<int>(0, (srcLeft + srcRight) >> 1, sw - 1);
          const int srcY = qBound<int>(0, (srcTop + srcBottom) >> 1, sh - 1);
          dstLine[dx] = srcData[srcY * srcStride + srcX];
        }
        continue;
      }

      /*
       * Note that (intval / 32) is not the same as (intval >> 5).
       * The former rounds towards zero, while the latter rounds towards
       * negative infinity.
       * Likewise, (intval % 32) is not the same as (intval & 31).
       * The following expression:
       * topFraction = 32 - (src32Top & 31);
       * works correctly with both positive and negative src32Top.
       */

      unsigned backgroundArea = 0;

      if (srcTop < 0) {
        const unsigned topFraction = 32 - (src32Top & 31);
        const unsigned horFraction = src32Right - src32Left;
        backgroundArea += topFraction * horFraction;
        const unsigned fullPixelsVer = -1 - srcTop;
        backgroundArea += horFraction * (fullPixelsVer << 5);
        srcTop = 0;
        src32Top = 0;
      }
      if (srcBottom >= sh) {
        const unsigned bottomFraction = src32Bottom - (srcBottom << 5);
        const unsigned horFraction = src32Right - src32Left;
        backgroundArea += bottomFraction * horFraction;
        const unsigned fullPixelsVer = srcBottom - sh;
        backgroundArea += horFraction * (fullPixelsVer << 5);
        srcBottom = sh - 1;     // inclusive
        src32Bottom = sh << 5;  // exclusive
      }
      if (srcLeft < 0) {
        const unsigned leftFraction = 32 - (src32Left & 31);
        const unsigned vertFraction = src32Bottom - src32Top;
        backgroundArea += leftFraction * vertFraction;
        const unsigned fullPixelsHor = -1 - srcLeft;
        backgroundArea += vertFraction * (fullPixelsHor << 5);
        srcLeft = 0;
        src32Left = 0;
      }
      if (srcRight >= sw) {
        const unsigned rightFraction = src32Right - (srcRight << 5);
        const unsigned vertFraction = src32Bottom - src32Top;
        backgroundArea += rightFraction * vertFraction;
        const unsigned fullPixelsHor = srcRight - sw;
        backgroundArea += vertFraction * (fullPixelsHor << 5);
        srcRight = sw - 1;     // inclusive
        src32Right = sw << 5;  // exclusive
      }
      assert(srcBottom >= srcTop);
      assert(srcRight >= srcLeft);

      Mixer mixer;
      if (outsideFlags & OutsidePixels::WEAK) {
        backgroundArea = 0;
      } else {
        assert(outsideFlags & OutsidePixels::COLOR);
        mixer.add(outsideColor, backgroundArea);
      }

      const unsigned leftFraction = 32 - (src32Left & 31);
      const unsigned topFraction = 32 - (src32Top & 31);
      const unsigned rightFraction = src32Right - (srcRight << 5);
      const unsigned bottomFraction = src32Bottom - (srcBottom << 5);

      assert(leftFraction + rightFraction + (srcRight - srcLeft - 1) * 32
             == static_cast<unsigned>(src32Right - src32Left));
      assert(topFraction + bottomFraction + (srcBottom - srcTop - 1) * 32
             == static_cast<unsigned>(src32Bottom - src32Top));

      const unsigned srcArea = (src32Bottom - src32Top) * (src32Right - src32Left);
      if (srcArea == 0) {
        if ((outsideFlags & OutsidePixels::COLOR)) {
          dstLine[dx] = outsideColor;
        } else {
          const int srcX = qBound<int>(0, (srcLeft + srcRight) >> 1, sw - 1);
          const int srcY = qBound<int>(0, (srcTop + srcBottom) >> 1, sh - 1);
          dstLine[dx] = srcData[srcY * srcStride + srcX];
        }
        continue;
      }

      const StorageUnit* srcLine = &srcData[srcTop * srcStride];

      if (srcTop == srcBottom) {
        if (srcLeft == srcRight) {
          // dst pixel maps to a single src pixel
          const StorageUnit c = srcLine[srcLeft];
          if (backgroundArea == 0) {
            // common case optimization
            dstLine[dx] = c;
            continue;
          }
          mixer.add(c, srcArea);
        } else {
          // dst pixel maps to a horizontal line of src pixels
          const unsigned vertFraction = src32Bottom - src32Top;
          const unsigned leftArea = vertFraction * leftFraction;
          const unsigned middleArea = vertFraction << 5;
          const unsigned rightArea = vertFraction * rightFraction;

          mixer.add(srcLine[srcLeft], leftArea);

          for (int sx = srcLeft + 1; sx < srcRight; ++sx) {
            mixer.add(srcLine[sx], middleArea);
          }

          mixer.add(srcLine[srcRight], rightArea);
        }
      } else if (srcLeft == srcRight) {
        // dst pixel maps to a vertical line of src pixels
        const unsigned horFraction = src32Right - src32Left;
        const unsigned topArea = horFraction * topFraction;
        const unsigned middleArea = horFraction << 5;
        const unsigned bottomArea = horFraction * bottomFraction;

        srcLine += srcLeft;
        mixer.add(*srcLine, topArea);

        srcLine += srcStride;

        for (int sy = srcTop + 1; sy < srcBottom; ++sy) {
          mixer.add(*srcLine, middleArea);
          srcLine += srcStride;
        }

        mixer.add(*srcLine, bottomArea);
      } else {
        // dst pixel maps to a block of src pixels
        const unsigned topArea = topFraction << 5;
        const unsigned bottomArea = bottomFraction << 5;
        const unsigned leftArea = leftFraction << 5;
        const unsigned rightArea = rightFraction << 5;
        const unsigned topleftArea = topFraction * leftFraction;
        const unsigned toprightArea = topFraction * rightFraction;
        const unsigned bottomleftArea = bottomFraction * leftFraction;
        const unsigned bottomrightArea = bottomFraction * rightFraction;

        // process the top-left corner
        mixer.add(srcLine[srcLeft], topleftArea);

        // process the top line (without corners)
        for (int sx = srcLeft + 1; sx < srcRight; ++sx) {
          mixer.add(srcLine[sx], topArea);
        }

        // process the top-right corner
        mixer.add(srcLine[srcRight], toprightArea);

        srcLine += srcStride;
        // process middle lines
        for (int sy = srcTop + 1; sy < srcBottom; ++sy) {
          mixer.add(srcLine[srcLeft], leftArea);

          for (int sx = srcLeft + 1; sx < srcRight; ++sx) {
            mixer.add(srcLine[sx], 32 * 32);
          }

          mixer.add(srcLine[srcRight], rightArea);

          srcLine += srcStride;
        }

        // process bottom-left corner
        mixer.add(srcLine[srcLeft], bottomleftArea);

        // process the bottom line (without corners)
        for (int sx = srcLeft + 1; sx < srcRight; ++sx) {
          mixer.add(srcLine[sx], bottomArea);
        }

        // process the bottom-right corner
        mixer.add(srcLine[srcRight], bottomrightArea);
      }

      dstLine[dx] = mixer.mix(srcArea + backgroundArea);
    }
  }
}  // transformGeneric

template <typename ImageT>
void fixDpiInPlace(ImageT& dst, const QImage& src, const QTransform& xform) {
  QLineF horLine(0, 0, src.dotsPerMeterX(), 0);
  QLineF verLine(0, 0, 0, src.dotsPerMeterY());
  dst.setDotsPerMeterX(qRound(xform.map(horLine).length()));
  dst.setDotsPerMeterY(qRound(xform.map(verLine).length()));
}
}  // namespace

QImage transform(const QImage& src,
                 const QTransform& xform,
                 const QRect& dstRect,
                 const OutsidePixels outsidePixels,
                 const QSizeF& minMappingArea) {
  if (src.isNull() || dstRect.isEmpty()) {
    return QImage();
  }
  if (!xform.isAffine()) {
    throw std::invalid_argument("transform: only affine transformations are supported");
  }
  if (!dstRect.isValid()) {
    throw std::invalid_argument("transform: dstRect is invalid");
  }

  auto isOpaqueGray
      = [](QRgb rgba) { return qAlpha(rgba) == 0xff && qRed(rgba) == qBlue(rgba) && qRed(rgba) == qGreen(rgba); };
  switch (src.format()) {
    case QImage::Format_Invalid:
      return QImage();
    case QImage::Format_Indexed8:
    case QImage::Format_Mono:
    case QImage::Format_MonoLSB:
      if (src.allGray() && isOpaqueGray(outsidePixels.rgba())) {
        // The palette of src may be non-standard, so we create a GrayImage,
        // which is guaranteed to have a standard palette.
        GrayImage graySrc(src);
        GrayImage grayDst(dstRect.size());
        using AccumType = uint32_t;
        transformGeneric<uint8_t, GrayColorMixer<AccumType>>(
            graySrc.data(), graySrc.stride(), src.size(), grayDst.data(), grayDst.stride(), xform, dstRect,
            outsidePixels.grayLevel(), outsidePixels.flags(), minMappingArea);

        fixDpiInPlace(grayDst, src, xform);
        return grayDst;
      }
      // fall through
    default:
      if (!src.hasAlphaChannel() && (qAlpha(outsidePixels.rgba()) == 0xff)) {
        const QImage srcRgb32(src.convertToFormat(QImage::Format_RGB32));
        badAllocIfNull(srcRgb32);
        QImage dst(dstRect.size(), QImage::Format_RGB32);
        badAllocIfNull(dst);

        using AccumType = uint32_t;
        transformGeneric<uint32_t, RgbColorMixer<AccumType>>(
            (const uint32_t*) srcRgb32.bits(), srcRgb32.bytesPerLine() / 4, srcRgb32.size(), (uint32_t*) dst.bits(),
            dst.bytesPerLine() / 4, xform, dstRect, outsidePixels.rgb(), outsidePixels.flags(), minMappingArea);

        fixDpiInPlace(dst, src, xform);
        return dst;
      } else {
        const QImage srcArgb32(src.convertToFormat(QImage::Format_ARGB32));
        badAllocIfNull(srcArgb32);
        QImage dst(dstRect.size(), QImage::Format_ARGB32);
        badAllocIfNull(dst);

        using AccumType = float;
        transformGeneric<uint32_t, ArgbColorMixer<AccumType>>(
            (const uint32_t*) srcArgb32.bits(), srcArgb32.bytesPerLine() / 4, srcArgb32.size(), (uint32_t*) dst.bits(),
            dst.bytesPerLine() / 4, xform, dstRect, outsidePixels.rgba(), outsidePixels.flags(), minMappingArea);

        fixDpiInPlace(dst, src, xform);
        return dst;
      }
  }
}  // transform

GrayImage transformToGray(const QImage& src,
                          const QTransform& xform,
                          const QRect& dstRect,
                          const OutsidePixels outsidePixels,
                          const QSizeF& minMappingArea) {
  if (src.isNull() || dstRect.isEmpty()) {
    return GrayImage();
  }
  if (!xform.isAffine()) {
    throw std::invalid_argument("transformToGray: only affine transformations are supported");
  }
  if (!dstRect.isValid()) {
    throw std::invalid_argument("transformToGray: dstRect is invalid");
  }

  const GrayImage graySrc(src);
  GrayImage dst(dstRect.size());

  using AccumType = unsigned;
  transformGeneric<uint8_t, GrayColorMixer<AccumType>>(graySrc.data(), graySrc.stride(), graySrc.size(), dst.data(),
                                                       dst.stride(), xform, dstRect, outsidePixels.grayLevel(),
                                                       outsidePixels.flags(), minMappingArea);

  fixDpiInPlace(dst, src, xform);
  return dst;
}
}  // namespace imageproc