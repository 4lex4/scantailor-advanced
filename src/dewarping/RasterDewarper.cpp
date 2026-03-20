// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "RasterDewarper.h"

#include <ColorMixer.h>
#include <GrayImage.h>

#include <QDebug>
#include <cmath>
#include <stdexcept>

#include "CylindricalSurfaceDewarper.h"

#define INTERP_NONE 0
#define INTERP_BILLINEAR 1
#define INTERP_AREA_MAPPING 2
#define INTERPOLATION_METHOD INTERP_AREA_MAPPING

using namespace imageproc;

namespace dewarping {
namespace {
#if INTERPOLATION_METHOD == INTERP_NONE
template <typename ColorMixer, typename PixelType>
void dewarpGeneric(const PixelType* const srcData,
                   const QSize srcSize,
                   const int srcStride,
                   PixelType* const dstData,
                   const QSize dstSize,
                   const int dstStride,
                   const CylindricalSurfaceDewarper& distortionModel,
                   const QRectF& modelDomain,
                   const PixelType bgColor) {
  const int srcWidth = srcSize.width();
  const int srcHeight = srcSize.height();
  const int dstWidth = dstSize.width();
  const int dstHeight = dstSize.height();

  CylindricalSurfaceDewarper::State state;

  const double modelDomainLeft = modelDomain.left();
  const double modelXScale = 1.0 / (modelDomain.right() - modelDomain.left());

  const float modelDomainTop = modelDomain.top();
  const float modelYScale = 1.0 / (modelDomain.bottom() - modelDomain.top());

  for (int dstX = 0; dstX < dstWidth; ++dstX) {
    const double modelX = (dstX - modelDomainLeft) * modelXScale;
    const CylindricalSurfaceDewarper::Generatrix generatrix(distortionModel.mapGeneratrix(modelX, state));

    const HomographicTransform<1, float> homog(generatrix.pln2img.mat());
    const Vec2f origin(generatrix.imgLine.p1());
    const Vec2f vec(generatrix.imgLine.p2() - generatrix.imgLine.p1());
    for (int dstY = 0; dstY < dstHeight; ++dstY) {
      const float modelY = (float(dstY) - modelDomainTop) * modelYScale;
      const Vec2f srcPt(origin + vec * homog(modelY));
      const int srcX = qRound(srcPt[0]);
      const int srcY = qRound(srcPt[1]);
      if ((srcX < 0) || (srcX >= srcWidth) || (srcY < 0) || (srcY >= srcHeight)) {
        dstData[dstY * dstStride + dstX] = bgColor;
        continue;
      }

      dstData[dstY * dstStride + dstX] = srcData[srcY * srcStride + srcX];
    }
  }
}  // dewarpGeneric

#elif INTERPOLATION_METHOD == INTERP_BILLINEAR
template <typename ColorMixer, typename PixelType>
void dewarpGeneric(const PixelType* const srcData,
                   const QSize srcSize,
                   const int srcStride,
                   PixelType* const dstData,
                   const QSize dstSize,
                   const int dstStride,
                   const CylindricalSurfaceDewarper& distortionModel,
                   const QRectF& modelDomain,
                   const PixelType bgColor) {
  const int srcWidth = srcSize.width();
  const int srcHeight = srcSize.height();
  const int dstWidth = dstSize.width();
  const int dstHeight = dstSize.height();

  CylindricalSurfaceDewarper::State state;

  const double modelDomainLeft = modelDomain.left() - 0.5f;
  const double modelXScale = 1.0 / (modelDomain.right() - modelDomain.left());

  const float modelDomainTop = modelDomain.top() - 0.5f;
  const float modelYScale = 1.0 / (modelDomain.bottom() - modelDomain.top());

  for (int dstX = 0; dstX < dstWidth; ++dstX) {
    const double modelX = (dstX - modelDomainLeft) * modelXScale;
    const CylindricalSurfaceDewarper::Generatrix generatrix(distortionModel.mapGeneratrix(modelX, state));

    const HomographicTransform<1, float> homog(generatrix.pln2img.mat());
    const Vec2f origin(generatrix.imgLine.p1());
    const Vec2f vec(generatrix.imgLine.p2() - generatrix.imgLine.p1());
    for (int dstY = 0; dstY < dstHeight; ++dstY) {
      const float modelY = ((float) dstY - modelDomainTop) * modelYScale;
      const Vec2f srcPt(origin + vec * homog(modelY));

      const int srcX0 = (int) std::floor(srcPt[0] - 0.5f);
      const int srcY0 = (int) std::floor(srcPt[1] - 0.5f);
      const int srcX1 = srcX0 + 1;
      const int srcY1 = srcY0 + 1;
      const float x = srcPt[0] - srcX0;
      const float y = srcPt[1] - srcY0;

      PixelType tlColor = bgColor;
      if ((srcX0 >= 0) && (srcX0 < srcWidth) && (srcY0 >= 0) && (srcY0 < srcHeight)) {
        tlColor = srcData[srcY0 * srcStride + srcX0];
      }

      PixelType trColor = bgColor;
      if ((srcX1 >= 0) && (srcX1 < srcWidth) && (srcY0 >= 0) && (srcY0 < srcHeight)) {
        trColor = srcData[srcY0 * srcStride + srcX1];
      }

      PixelType blColor = bgColor;
      if ((srcX0 >= 0) && (srcX0 < srcWidth) && (srcY1 >= 0) && (srcY1 < srcHeight)) {
        blColor = srcData[srcY1 * srcStride + srcX0];
      }

      PixelType brColor = bgColor;
      if ((srcX1 >= 0) && (srcX1 < srcWidth) && (srcY1 >= 0) && (srcY1 < srcHeight)) {
        brColor = srcData[srcY1 * srcStride + srcX1];
      }

      ColorMixer mixer;
      mixer.add(tlColor, (1.5f - y) * (1.5f - x));
      mixer.add(trColor, (1.5f - y) * (x - 0.5f));
      mixer.add(blColor, (y - 0.5f) * (1.5f - x));
      mixer.add(brColor, (y - 0.5f) * (x - 0.5f));
      dstData[dstY * dstStride + dstX] = mixer.mix(1.0f);
    }
  }
}  // dewarpGeneric

#elif INTERPOLATION_METHOD == INTERP_AREA_MAPPING

template <typename ColorMixer, typename PixelType>
void areaMapGeneratrix(const PixelType* const srcData,
                       const QSize srcSize,
                       const int srcStride,
                       PixelType* pDst,
                       const QSize dstSize,
                       const int dstStride,
                       const PixelType bgColor,
                       const std::vector<Vec2f>& prevGridColumn,
                       const std::vector<Vec2f>& nextGridColumn) {
  const int sw = srcSize.width();
  const int sh = srcSize.height();
  const int dstHeight = dstSize.height();

  const Vec2f* srcLeftPoints = &prevGridColumn[0];
  const Vec2f* srcRightPoints = &nextGridColumn[0];

  Vec2f f_src32_quad[4];

  for (int dstY = 0; dstY < dstHeight; ++dstY) {
    // Take a mid-point of each edge, pre-multiply by 32,
    // write the result to f_src32_quad. 16 comes from 32*0.5
    f_src32_quad[0] = 16.0f * (srcLeftPoints[0] + srcRightPoints[0]);
    f_src32_quad[1] = 16.0f * (srcRightPoints[0] + srcRightPoints[1]);
    f_src32_quad[2] = 16.0f * (srcRightPoints[1] + srcLeftPoints[1]);
    f_src32_quad[3] = 16.0f * (srcLeftPoints[0] + srcLeftPoints[1]);
    ++srcLeftPoints;
    ++srcRightPoints;

    // Calculate the bounding box of src_quad.

    float fSrc32Left = f_src32_quad[0][0];
    float fSrc32Top = f_src32_quad[0][1];
    float fSrc32Right = fSrc32Left;
    float fSrc32Bottom = fSrc32Top;

    for (int i = 1; i < 4; ++i) {
      const Vec2f pt(f_src32_quad[i]);
      if (pt[0] < fSrc32Left) {
        fSrc32Left = pt[0];
      } else if (pt[0] > fSrc32Right) {
        fSrc32Right = pt[0];
      }
      if (pt[1] < fSrc32Top) {
        fSrc32Top = pt[1];
      } else if (pt[1] > fSrc32Bottom) {
        fSrc32Bottom = pt[1];
      }
    }

    if ((fSrc32Top < -32.0f * 10000.0f) || (fSrc32Left < -32.0f * 10000.0f)
        || (fSrc32Bottom > 32.0f * (float(sh) + 10000.f)) || (fSrc32Right > 32.0f * (float(sw) + 10000.f))) {
      // This helps to prevent integer overflows.
      *pDst = bgColor;
      pDst += dstStride;
      continue;
    }

    // Note: the code below is more or less the same as in transformGeneric()
    // in imageproc/Transform.cpp

    // Note that without using std::floor() and std::ceil()
    // we can't guarantee that srcBottom >= srcTop
    // and srcRight >= srcLeft.
    auto src32Left = (int) std::floor(fSrc32Left);
    auto src32Right = (int) std::ceil(fSrc32Right);
    auto src32Top = (int) std::floor(fSrc32Top);
    auto src32Bottom = (int) std::ceil(fSrc32Bottom);
    int srcLeft = src32Left >> 5;
    int srcRight = (src32Right - 1) >> 5;  // inclusive
    int srcTop = src32Top >> 5;
    int srcBottom = (src32Bottom - 1) >> 5;  // inclusive
    assert(srcBottom >= srcTop);
    assert(srcRight >= srcLeft);

    if ((srcBottom < 0) || (srcRight < 0) || (srcLeft >= sw) || (srcTop >= sh)) {
      // Completely outside of src image.
      *pDst = bgColor;
      pDst += dstStride;
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

    ColorMixer mixer;
    // if (weak_background) {
    // backgroundArea = 0;
    // } else {
    mixer.add(bgColor, backgroundArea);
    // }

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
      *pDst = bgColor;
      pDst += dstStride;
      continue;
    }

    const PixelType* srcLine = &srcData[srcTop * srcStride];

    if (srcTop == srcBottom) {
      if (srcLeft == srcRight) {
        // dst pixel maps to a single src pixel
        const PixelType c = srcLine[srcLeft];
        if (backgroundArea == 0) {
          // common case optimization
          *pDst = c;
          pDst += dstStride;
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

    *pDst = mixer.mix(srcArea + backgroundArea);
    pDst += dstStride;
  }
}  // areaMapGeneratrix

template <typename ColorMixer, typename PixelType>
void dewarpGeneric(const PixelType* const srcData,
                   const QSize srcSize,
                   const int srcStride,
                   PixelType* const dstData,
                   const QSize dstSize,
                   const int dstStride,
                   const CylindricalSurfaceDewarper& distortionModel,
                   const QRectF& modelDomain,
                   const PixelType bgColor) {
  const int srcWidth = srcSize.width();
  const int srcHeight = srcSize.height();
  const int dstWidth = dstSize.width();
  const int dstHeight = dstSize.height();

  CylindricalSurfaceDewarper::State state;

  const double modelDomainLeft = modelDomain.left();
  const double modelXScale = 1.0 / (modelDomain.right() - modelDomain.left());

  const auto modelDomainTop = static_cast<float>(modelDomain.top());
  const auto modelYScale = static_cast<float>(1.0 / (modelDomain.bottom() - modelDomain.top()));

  std::vector<Vec2f> prevGridColumn(dstHeight + 1);
  std::vector<Vec2f> nextGridColumn(dstHeight + 1);

  for (int dstX = 0; dstX <= dstWidth; ++dstX) {
    const double modelX = (dstX - modelDomainLeft) * modelXScale;
    const CylindricalSurfaceDewarper::Generatrix generatrix(distortionModel.mapGeneratrix(modelX, state));

    const HomographicTransform<1, float> homog(generatrix.pln2img.mat());
    const Vec2f origin(generatrix.imgLine.p1());
    const Vec2f vec(generatrix.imgLine.p2() - generatrix.imgLine.p1());
    for (int dstY = 0; dstY <= dstHeight; ++dstY) {
      const float modelY = (float(dstY) - modelDomainTop) * modelYScale;
      nextGridColumn[dstY] = origin + vec * homog(modelY);
    }

    if (dstX != 0) {
      areaMapGeneratrix<ColorMixer, PixelType>(srcData, srcSize, srcStride, dstData + dstX - 1, dstSize, dstStride,
                                               bgColor, prevGridColumn, nextGridColumn);
    }

    prevGridColumn.swap(nextGridColumn);
  }
}  // dewarpGeneric
#endif  // INTERPOLATION_METHOD
#if INTERPOLATION_METHOD == INTERP_BILLINEAR
using MixingWeight = float;
#else
using MixingWeight = unsigned;
#endif

QImage dewarpGrayscale(const QImage& src,
                       const QSize& dstSize,
                       const CylindricalSurfaceDewarper& distortionModel,
                       const QRectF& modelDomain,
                       const QColor& bgColor) {
  GrayImage dst(dstSize);
  const auto bgSample = static_cast<uint8_t>(qGray(bgColor.rgb()));
  dst.fill(bgSample);
  dewarpGeneric<GrayColorMixer<MixingWeight>, uint8_t>(src.bits(), src.size(), src.bytesPerLine(), dst.data(), dstSize,
                                                       dst.stride(), distortionModel, modelDomain, bgSample);
  return dst.toQImage();
}

QImage dewarpRgb(const QImage& src,
                 const QSize& dstSize,
                 const CylindricalSurfaceDewarper& distortionModel,
                 const QRectF& modelDomain,
                 const QColor& bgColor) {
  QImage dst(dstSize, QImage::Format_RGB32);
  dst.fill(bgColor.rgb());
  dewarpGeneric<RgbColorMixer<MixingWeight>, uint32_t>((const uint32_t*) src.bits(), src.size(), src.bytesPerLine() / 4,
                                                       (uint32_t*) dst.bits(), dstSize, dst.bytesPerLine() / 4,
                                                       distortionModel, modelDomain, bgColor.rgb());
  return dst;
}

QImage dewarpArgb(const QImage& src,
                  const QSize& dstSize,
                  const CylindricalSurfaceDewarper& distortionModel,
                  const QRectF& modelDomain,
                  const QColor& bgColor) {
  QImage dst(dstSize, QImage::Format_ARGB32);
  dst.fill(bgColor.rgba());
  dewarpGeneric<ArgbColorMixer<MixingWeight>, uint32_t>(
      (const uint32_t*) src.bits(), src.size(), src.bytesPerLine() / 4, (uint32_t*) dst.bits(), dstSize,
      dst.bytesPerLine() / 4, distortionModel, modelDomain, bgColor.rgba());
  return dst;
}
}  // namespace

QImage RasterDewarper::dewarp(const QImage& src,
                              const QSize& dstSize,
                              const CylindricalSurfaceDewarper& distortionModel,
                              const QRectF& modelDomain,
                              const QColor& bgColor) {
  if (modelDomain.isEmpty()) {
    throw std::invalid_argument("RasterDewarper: modelDomain is empty.");
  }

  switch (src.format()) {
    case QImage::Format_Invalid:
      return QImage();
    case QImage::Format_RGB32:
      return dewarpRgb(src, dstSize, distortionModel, modelDomain, bgColor);
    case QImage::Format_ARGB32:
      return dewarpArgb(src, dstSize, distortionModel, modelDomain, bgColor);
    case QImage::Format_Indexed8:
      if (src.isGrayscale()) {
        return dewarpGrayscale(src, dstSize, distortionModel, modelDomain, bgColor);
      } else if (src.allGray()) {
        // Only shades of gray but non-standard palette.
        return dewarpGrayscale(GrayImage(src).toQImage(), dstSize, distortionModel, modelDomain, bgColor);
      }
      break;
    case QImage::Format_Mono:
    case QImage::Format_MonoLSB:
      if (src.allGray()) {
        return dewarpGrayscale(GrayImage(src).toQImage(), dstSize, distortionModel, modelDomain, bgColor);
      }
      break;
    default:;
  }
  // Generic case: convert to either RGB32 or ARGB32.
  if (src.hasAlphaChannel()) {
    return dewarpArgb(src.convertToFormat(QImage::Format_ARGB32), dstSize, distortionModel, modelDomain, bgColor);
  } else {
    return dewarpRgb(src.convertToFormat(QImage::Format_RGB32), dstSize, distortionModel, modelDomain, bgColor);
  }
}  // RasterDewarper::dewarp
}  // namespace dewarping
