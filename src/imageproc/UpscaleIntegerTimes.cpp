// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "UpscaleIntegerTimes.h"

#include <stdexcept>

#include "BinaryImage.h"

namespace imageproc {
namespace {
inline uint32_t multiplyBit(uint32_t bit, int times) {
  return (uint32_t(0) - bit) >> (32 - times);
}

void expandImpl(BinaryImage& dst, const BinaryImage& src, const int xscale, const int yscale) {
  const int sw = src.width();
  const int sh = src.height();

  const int srcWpl = src.wordsPerLine();
  const int dstWpl = dst.wordsPerLine();

  const uint32_t* srcLine = src.data();
  uint32_t* dstLine = dst.data();

  for (int sy = 0; sy < sh; ++sy, srcLine += srcWpl) {
    uint32_t dstWord = 0;
    int dstBitsRemaining = 32;
    int di = 0;

    for (int sx = 0; sx < sw; ++sx) {
      const uint32_t srcWord = srcLine[sx >> 5];
      const int srcBit = 31 - (sx & 31);
      const uint32_t bit = (srcWord >> srcBit) & uint32_t(1);
      int todo = xscale;

      while (dstBitsRemaining <= todo) {
        dstWord |= multiplyBit(bit, dstBitsRemaining);
        dstLine[di++] = dstWord;
        todo -= dstBitsRemaining;
        dstBitsRemaining = 32;
        dstWord = 0;
      }
      if (todo > 0) {
        dstBitsRemaining -= todo;
        dstWord |= multiplyBit(bit, todo) << dstBitsRemaining;
      }
    }

    if (dstBitsRemaining != 32) {
      dstLine[di] = dstWord;
    }

    const uint32_t* firstDstLine = dstLine;
    dstLine += dstWpl;
    for (int line = 1; line < yscale; ++line, dstLine += dstWpl) {
      memcpy(dstLine, firstDstLine, dstWpl * 4);
    }
  }
}  // expandImpl
}  // namespace

BinaryImage upscaleIntegerTimes(const BinaryImage& src, const int xscale, const int yscale) {
  if (src.isNull() || ((xscale == 1) && (yscale == 1))) {
    return src;
  }

  if ((xscale < 0) || (yscale < 0)) {
    throw std::runtime_error("upscaleIntegerTimes: scaling factors can't be negative");
  }

  BinaryImage dst(src.width() * xscale, src.height() * yscale);
  expandImpl(dst, src, xscale, yscale);
  return dst;
}

BinaryImage upscaleIntegerTimes(const BinaryImage& src, const QSize& dstSize, const BWColor padding) {
  if (src.isNull()) {
    BinaryImage dst(dstSize);
    dst.fill(padding);
    return dst;
  }

  const int xscale = dstSize.width() / src.width();
  const int yscale = dstSize.height() / src.height();
  if ((xscale < 1) || (yscale < 1)) {
    throw std::invalid_argument("upscaleIntegerTimes: bad dstSize");
  }

  BinaryImage dst(dstSize);
  expandImpl(dst, src, xscale, yscale);
  const QRect rect(0, 0, src.width() * xscale, src.height() * yscale);
  dst.fillExcept(rect, padding);
  return dst;
}
}  // namespace imageproc
