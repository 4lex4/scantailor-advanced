// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "DrawOver.h"
#include <QImage>
#include <cassert>
#include "BinaryImage.h"
#include "RasterOp.h"

namespace imageproc {
void drawOver(QImage& dst, const QRect& dstRect, const QImage& src, const QRect& srcRect) {
  if (srcRect.size() != dstRect.size()) {
    throw std::invalid_argument("drawOver: source and destination areas have different sizes");
  }
  if (dst.format() != src.format()) {
    throw std::invalid_argument("drawOver: source and destination have different formats");
  }
  if (dstRect.intersected(dst.rect()) != dstRect) {
    throw std::invalid_argument("drawOver: destination area exceeds the image");
  }
  if (srcRect.intersected(src.rect()) != srcRect) {
    throw std::invalid_argument("drawOver: source area exceeds the image");
  }

  uint8_t* dstLine = dst.bits();
  const int dstBpl = dst.bytesPerLine();

  const uint8_t* srcLine = src.bits();
  const int srcBpl = src.bytesPerLine();

  const int depth = src.depth();
  assert(dst.depth() == depth);

  if (depth == 1) {
    // Slow but simple.
    BinaryImage dstBin(dst);
    BinaryImage srcBin(src);
    rasterOp<RopSrc>(dstBin, dstRect, srcBin, srcRect.topLeft());
    dst = dstBin.toQImage().convertToFormat(dst.format());

    return;
  }

  const int stripeBytes = srcRect.width() * depth / 8;
  dstLine += dstBpl * dstRect.top() + dstRect.left() * depth / 8;
  srcLine += srcBpl * srcRect.top() + srcRect.left() * depth / 8;

  for (int i = srcRect.height(); i > 0; --i) {
    memcpy(dstLine, srcLine, stripeBytes);
    dstLine += dstBpl;
    srcLine += srcBpl;
  }
}  // drawOver
}  // namespace imageproc