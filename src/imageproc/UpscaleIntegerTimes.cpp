/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2007-2009  Joseph Artsimovich <joseph_a@mail.ru>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "UpscaleIntegerTimes.h"
#include "BinaryImage.h"

namespace imageproc {
namespace {
inline uint32_t multiplyBit(uint32_t bit, int times) {
  return (uint32_t(0) - bit) >> (32 - times);
}

void expandImpl(BinaryImage& dst, const BinaryImage& src, const int xscale, const int yscale) {
  const int sw = src.width();
  const int sh = src.height();

  const int src_wpl = src.wordsPerLine();
  const int dst_wpl = dst.wordsPerLine();

  const uint32_t* src_line = src.data();
  uint32_t* dst_line = dst.data();

  for (int sy = 0; sy < sh; ++sy, src_line += src_wpl) {
    uint32_t dst_word = 0;
    int dst_bits_remaining = 32;
    int di = 0;

    for (int sx = 0; sx < sw; ++sx) {
      const uint32_t src_word = src_line[sx >> 5];
      const int src_bit = 31 - (sx & 31);
      const uint32_t bit = (src_word >> src_bit) & uint32_t(1);
      int todo = xscale;

      while (dst_bits_remaining <= todo) {
        dst_word |= multiplyBit(bit, dst_bits_remaining);
        dst_line[di++] = dst_word;
        todo -= dst_bits_remaining;
        dst_bits_remaining = 32;
        dst_word = 0;
      }
      if (todo > 0) {
        dst_bits_remaining -= todo;
        dst_word |= multiplyBit(bit, todo) << dst_bits_remaining;
      }
    }

    if (dst_bits_remaining != 32) {
      dst_line[di] = dst_word;
    }

    const uint32_t* first_dst_line = dst_line;
    dst_line += dst_wpl;
    for (int line = 1; line < yscale; ++line, dst_line += dst_wpl) {
      memcpy(dst_line, first_dst_line, dst_wpl * 4);
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

BinaryImage upscaleIntegerTimes(const BinaryImage& src, const QSize& dst_size, const BWColor padding) {
  if (src.isNull()) {
    BinaryImage dst(dst_size);
    dst.fill(padding);

    return dst;
  }

  const int xscale = dst_size.width() / src.width();
  const int yscale = dst_size.height() / src.height();
  if ((xscale < 1) || (yscale < 1)) {
    throw std::invalid_argument("upscaleIntegerTimes: bad dst_size");
  }

  BinaryImage dst(dst_size);
  expandImpl(dst, src, xscale, yscale);
  const QRect rect(0, 0, src.width() * xscale, src.height() * yscale);
  dst.fillExcept(rect, padding);

  return dst;
}
}  // namespace imageproc