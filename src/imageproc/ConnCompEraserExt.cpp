// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "ConnCompEraserExt.h"
#include "RasterOp.h"

namespace imageproc {
ConnCompEraserExt::ConnCompEraserExt(const BinaryImage& image, const Connectivity conn)
    : m_eraser(image, conn), m_lastImage(image) {}

ConnComp ConnCompEraserExt::nextConnComp() {
  if (!m_lastCC.isNull()) {
    // Propagate the changes from m_eraser.image() to m_lastImage.
    // We could copy the whole image, but instead we copy just
    // the affected area, extending it to word boundries.
    const QRect& rect = m_lastCC.rect();
    const BinaryImage& src = m_eraser.image();
    const size_t srcWpl = src.wordsPerLine();
    const size_t dstWpl = m_lastImage.wordsPerLine();
    const size_t firstWordIdx = rect.left() / 32;
    // Note: rect.right() == rect.x() + rect.width() - 1
    const size_t spanLength = (rect.right() + 31) / 32 - firstWordIdx;
    const size_t srcInitialOffset = rect.top() * srcWpl + firstWordIdx;
    const size_t dstInitialOffset = rect.top() * dstWpl + firstWordIdx;
    const uint32_t* srcPos = src.data() + srcInitialOffset;
    uint32_t* dstPos = m_lastImage.data() + dstInitialOffset;
    for (int i = rect.height(); i > 0; --i) {
      memcpy(dstPos, srcPos, spanLength * 4);
      srcPos += srcWpl;
      dstPos += dstWpl;
    }
  }

  m_lastCC = m_eraser.nextConnComp();

  return m_lastCC;
}

BinaryImage ConnCompEraserExt::computeConnCompImage() const {
  if (m_lastCC.isNull()) {
    return BinaryImage();
  }

  return computeDiffImage(m_lastCC.rect());
}

BinaryImage ConnCompEraserExt::computeConnCompImageAligned(QRect* rect) const {
  if (m_lastCC.isNull()) {
    return BinaryImage();
  }

  QRect r(m_lastCC.rect());
  r.setX((r.x() >> 5) << 5);
  if (rect) {
    *rect = r;
  }

  return computeDiffImage(r);
}

BinaryImage ConnCompEraserExt::computeDiffImage(const QRect& rect) const {
  BinaryImage diff(rect.width(), rect.height());
  rasterOp<RopSrc>(diff, diff.rect(), m_eraser.image(), rect.topLeft());
  rasterOp<RopXor<RopSrc, RopDst>>(diff, diff.rect(), m_lastImage, rect.topLeft());

  return diff;
}
}  // namespace imageproc