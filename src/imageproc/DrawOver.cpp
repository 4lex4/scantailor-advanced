// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "DrawOver.h"
#include <QImage>
#include <cassert>
#include "BinaryImage.h"
#include "RasterOp.h"

namespace imageproc {
void drawOver(QImage& dst, const QRect& dst_rect, const QImage& src, const QRect& src_rect) {
  if (src_rect.size() != dst_rect.size()) {
    throw std::invalid_argument("drawOver: source and destination areas have different sizes");
  }
  if (dst.format() != src.format()) {
    throw std::invalid_argument("drawOver: source and destination have different formats");
  }
  if (dst_rect.intersected(dst.rect()) != dst_rect) {
    throw std::invalid_argument("drawOver: destination area exceeds the image");
  }
  if (src_rect.intersected(src.rect()) != src_rect) {
    throw std::invalid_argument("drawOver: source area exceeds the image");
  }

  uint8_t* dst_line = dst.bits();
  const int dst_bpl = dst.bytesPerLine();

  const uint8_t* src_line = src.bits();
  const int src_bpl = src.bytesPerLine();

  const int depth = src.depth();
  assert(dst.depth() == depth);

  if (depth % 8 != 0) {
    assert(depth == 1);

    // Slow but simple.
    BinaryImage dst_bin(dst);
    BinaryImage src_bin(src);
    rasterOp<RopSrc>(dst_bin, dst_rect, src_bin, src_rect.topLeft());
    dst = dst_bin.toQImage().convertToFormat(dst.format());
    // FIXME: we are not preserving the color table.

    return;
  }

  const int stripe_bytes = src_rect.width() * depth / 8;
  dst_line += dst_bpl * dst_rect.top() + dst_rect.left() * depth / 8;
  src_line += src_bpl * src_rect.top() + src_rect.left() * depth / 8;

  for (int i = src_rect.height(); i > 0; --i) {
    memcpy(dst_line, src_line, stripe_bytes);
    dst_line += dst_bpl;
    src_line += src_bpl;
  }
}  // drawOver
}  // namespace imageproc