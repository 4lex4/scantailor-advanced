// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "ReduceThreshold.h"

#include <stdexcept>
#include <cassert>

namespace imageproc {
namespace {
/**
 * This lookup table is filled like this:
 * \code
 * for (unsigned i = 0; i < 256; i += 2) {
 *  unsigned out =
 *      ((i & (1 << 1)) >> 1) |  // bit 1 becomes bit 0
 *      ((i & (1 << 3)) >> 2) |  // bit 3 becomes bit 1
 *      ((i & (1 << 5)) >> 3) |  // bit 5 becomes bit 2
 *      ((i & (1 << 7)) >> 4);   // bit 7 becomes bit 3
 *  compressBitsLut[i >> 1] = static_cast<uint8_t>(out);
 * }
 * \endcode
 * We take every other byte because bit 0 doesn't matter here.
 */
const uint8_t compressBitsLut[128]
    = {0x0, 0x1, 0x0, 0x1, 0x2, 0x3, 0x2, 0x3, 0x0, 0x1, 0x0, 0x1, 0x2, 0x3, 0x2, 0x3, 0x4, 0x5, 0x4, 0x5, 0x6, 0x7,
       0x6, 0x7, 0x4, 0x5, 0x4, 0x5, 0x6, 0x7, 0x6, 0x7, 0x0, 0x1, 0x0, 0x1, 0x2, 0x3, 0x2, 0x3, 0x0, 0x1, 0x0, 0x1,
       0x2, 0x3, 0x2, 0x3, 0x4, 0x5, 0x4, 0x5, 0x6, 0x7, 0x6, 0x7, 0x4, 0x5, 0x4, 0x5, 0x6, 0x7, 0x6, 0x7, 0x8, 0x9,
       0x8, 0x9, 0xa, 0xb, 0xa, 0xb, 0x8, 0x9, 0x8, 0x9, 0xa, 0xb, 0xa, 0xb, 0xc, 0xd, 0xc, 0xd, 0xe, 0xf, 0xe, 0xf,
       0xc, 0xd, 0xc, 0xd, 0xe, 0xf, 0xe, 0xf, 0x8, 0x9, 0x8, 0x9, 0xa, 0xb, 0xa, 0xb, 0x8, 0x9, 0x8, 0x9, 0xa, 0xb,
       0xa, 0xb, 0xc, 0xd, 0xc, 0xd, 0xe, 0xf, 0xe, 0xf, 0xc, 0xd, 0xc, 0xd, 0xe, 0xf, 0xe, 0xf};

/**
 * Throw away every other bit starting with bit 0 and
 * pack the remaining bits into the upper half of a word.
 */
inline uint32_t compressBitsUpperHalf(const uint32_t bits) {
  uint32_t r;
  r = compressBitsLut[(bits >> 25) /*& 0x7F*/] << 28;
  r |= compressBitsLut[(bits >> 17) & 0x7F] << 24;
  r |= compressBitsLut[(bits >> 9) & 0x7F] << 20;
  r |= compressBitsLut[(bits >> 1) & 0x7F] << 16;
  return r;
}

/**
 * Throw away every other bit starting with bit 0 and
 * pack the remaining bits into the lower half of a word.
 */
inline uint32_t compressBitsLowerHalf(const uint32_t bits) {
  uint32_t r;
  r = compressBitsLut[(bits >> 25) /*& 0x7F*/] << 12;
  r |= compressBitsLut[(bits >> 17) & 0x7F] << 8;
  r |= compressBitsLut[(bits >> 9) & 0x7F] << 4;
  r |= compressBitsLut[(bits >> 1) & 0x7F];
  return r;
}

inline uint32_t threshold1(const uint32_t top, const uint32_t bottom) {
  uint32_t word = top | bottom;
  word |= word << 1;
  return word;
}

inline uint32_t threshold2(const uint32_t top, const uint32_t bottom) {
  uint32_t word1 = top & bottom;
  word1 |= word1 << 1;
  uint32_t word2 = top | bottom;
  word2 &= word2 << 1;
  return word1 | word2;
}

inline uint32_t threshold3(const uint32_t top, const uint32_t bottom) {
  uint32_t word1 = top | bottom;
  word1 &= word1 << 1;
  uint32_t word2 = top & bottom;
  word2 |= word2 << 1;
  return word1 & word2;
}

inline uint32_t threshold4(const uint32_t top, const uint32_t bottom) {
  uint32_t word = top & bottom;
  word &= word << 1;
  return word;
}
}  // namespace

ReduceThreshold::ReduceThreshold(const BinaryImage& image) : m_image(image) {}

ReduceThreshold& ReduceThreshold::reduce(const int threshold) {
  if ((threshold < 1) || (threshold > 4)) {
    throw std::invalid_argument("ReduceThreshold: invalid threshold");
  }

  const BinaryImage& src = m_image;

  if (src.isNull()) {
    return *this;
  }

  const int dstW = src.width() / 2;
  const int dstH = src.height() / 2;

  if (dstH == 0) {
    reduceHorLine(threshold);
    return *this;
  } else if (dstW == 0) {
    reduceVertLine(threshold);
    return *this;
  }

  BinaryImage dst(dstW, dstH);

  const int dstWpl = dst.wordsPerLine();
  const int srcWpl = src.wordsPerLine();
  const int stepsPerLine = (dstW * 2 + 31) / 32;
  assert(stepsPerLine <= srcWpl);
  assert(stepsPerLine / 2 <= dstWpl);

  const uint32_t* srcLine = src.data();
  uint32_t* dstLine = dst.data();

  uint32_t word;

  if (threshold == 1) {
    for (int i = dstH; i > 0; --i) {
      for (int j = 0; j < stepsPerLine; j += 2) {
        word = threshold1(srcLine[j], srcLine[j + srcWpl]);
        dstLine[j / 2] = compressBitsUpperHalf(word);
      }
      for (int j = 1; j < stepsPerLine; j += 2) {
        word = threshold1(srcLine[j], srcLine[j + srcWpl]);
        dstLine[j / 2] |= compressBitsLowerHalf(word);
      }
      srcLine += srcWpl * 2;
      dstLine += dstWpl;
    }
  } else if (threshold == 2) {
    for (int i = dstH; i > 0; --i) {
      for (int j = 0; j < stepsPerLine; j += 2) {
        word = threshold2(srcLine[j], srcLine[j + srcWpl]);
        dstLine[j / 2] = compressBitsUpperHalf(word);
      }
      for (int j = 1; j < stepsPerLine; j += 2) {
        word = threshold2(srcLine[j], srcLine[j + srcWpl]);
        dstLine[j / 2] |= compressBitsLowerHalf(word);
      }
      srcLine += srcWpl * 2;
      dstLine += dstWpl;
    }
  } else if (threshold == 3) {
    for (int i = dstH; i > 0; --i) {
      for (int j = 0; j < stepsPerLine; j += 2) {
        word = threshold3(srcLine[j], srcLine[j + srcWpl]);
        dstLine[j / 2] = compressBitsUpperHalf(word);
      }
      for (int j = 1; j < stepsPerLine; j += 2) {
        word = threshold3(srcLine[j], srcLine[j + srcWpl]);
        dstLine[j / 2] |= compressBitsLowerHalf(word);
      }
      srcLine += srcWpl * 2;
      dstLine += dstWpl;
    }
  } else if (threshold == 4) {
    for (int i = dstH; i > 0; --i) {
      for (int j = 0; j < stepsPerLine; j += 2) {
        word = threshold4(srcLine[j], srcLine[j + srcWpl]);
        dstLine[j / 2] = compressBitsUpperHalf(word);
      }
      for (int j = 1; j < stepsPerLine; j += 2) {
        word = threshold4(srcLine[j], srcLine[j + srcWpl]);
        dstLine[j / 2] |= compressBitsLowerHalf(word);
      }
      srcLine += srcWpl * 2;
      dstLine += dstWpl;
    }
  }

  m_image = dst;
  return *this;
}  // ReduceThreshold::reduce

void ReduceThreshold::reduceHorLine(const int threshold) {
  const BinaryImage& src = m_image;
  assert(src.height() == 1);

  if (src.width() == 1) {
    // 1x1 image remains the same no matter the threshold.
    return;
  }

  BinaryImage dst(src.width() / 2, 1);

  const int stepsPerLine = (dst.width() * 2 + 31) / 32;
  const uint32_t* srcLine = src.data();
  uint32_t* dstLine = dst.data();
  assert(stepsPerLine <= src.wordsPerLine());
  assert(stepsPerLine / 2 <= dst.wordsPerLine());

  uint32_t word;

  switch (threshold) {
    case 1:
    case 2:
      for (int j = 0; j < stepsPerLine; j += 2) {
        word = srcLine[j];
        word |= word << 1;
        dstLine[j / 2] = compressBitsUpperHalf(word);
      }
      for (int j = 1; j < stepsPerLine; j += 2) {
        word = srcLine[j];
        word |= word << 1;
        dstLine[j / 2] |= compressBitsLowerHalf(word);
      }
      break;
    case 3:
    case 4:
      for (int j = 0; j < stepsPerLine; j += 2) {
        word = srcLine[j];
        word &= word << 1;
        dstLine[j / 2] = compressBitsUpperHalf(word);
      }
      for (int j = 1; j < stepsPerLine; j += 2) {
        word = srcLine[j];
        word &= word << 1;
        dstLine[j / 2] |= compressBitsLowerHalf(word);
      }
      break;
    default:
      break;
  }

  m_image = dst;
}  // ReduceThreshold::reduceHorLine

void ReduceThreshold::reduceVertLine(const int threshold) {
  const BinaryImage& src = m_image;
  assert(src.width() == 1);

  if (src.height() == 1) {
    // 1x1 image remains the same no matter the threshold.
    return;
  }

  const int dstH = src.height() / 2;
  BinaryImage dst(1, dstH);

  const int srcWpl = src.wordsPerLine();
  const int dstWpl = dst.wordsPerLine();
  const uint32_t* srcLine = src.data();
  uint32_t* dstLine = dst.data();

  switch (threshold) {
    case 1:
    case 2:
      for (int i = dstH; i > 0; --i) {
        dstLine[0] = srcLine[0] | srcLine[srcWpl];
        srcLine += srcWpl * 2;
        dstLine += dstWpl;
      }
      break;
    case 3:
    case 4:
      for (int i = dstH; i > 0; --i) {
        dstLine[0] = srcLine[0] & srcLine[srcWpl];
        srcLine += srcWpl * 2;
        dstLine += dstWpl;
      }
      break;
    default:
      break;
  }

  m_image = dst;
}  // ReduceThreshold::reduceVertLine
}  // namespace imageproc
