// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "SeedFill.h"
#include <QDebug>
#include "GrayImage.h"
#include "SeedFillGeneric.h"

namespace imageproc {
namespace {
inline uint32_t fillWordHorizontally(uint32_t word, const uint32_t mask) {
  uint32_t prevWord;

  do {
    prevWord = word;
    word |= (word << 1) | (word >> 1);
    word &= mask;
  } while (word != prevWord);

  return word;
}

void seedFill4Iteration(BinaryImage& seed, const BinaryImage& mask) {
  const int w = seed.width();
  const int h = seed.height();

  const int seedWpl = seed.wordsPerLine();
  const int maskWpl = mask.wordsPerLine();
  const int lastWordIdx = (w - 1) >> 5;
  const uint32_t lastWordMask = ~uint32_t(0) << (((lastWordIdx + 1) << 5) - w);

  uint32_t* seedLine = seed.data();
  const uint32_t* maskLine = mask.data();
  const uint32_t* prevLine = seedLine;

  // Top to bottom.
  for (int y = 0; y < h; ++y) {
    uint32_t prevWord = 0;

    // Make sure offscreen bits are 0.
    seedLine[lastWordIdx] &= lastWordMask;
    // Left to right (except the last word).
    for (int i = 0; i <= lastWordIdx; ++i) {
      const uint32_t mask = maskLine[i];
      uint32_t word = prevWord << 31;
      word |= seedLine[i] | prevLine[i];
      word &= mask;
      word = fillWordHorizontally(word, mask);
      seedLine[i] = word;
      prevWord = word;
    }

    // If we don't do this, prevLine[lastWordIdx] on the next
    // iteration may contain garbage in the off-screen area.
    // That garbage can easily leak back.
    seedLine[lastWordIdx] &= lastWordMask;

    prevLine = seedLine;
    seedLine += seedWpl;
    maskLine += maskWpl;
  }

  seedLine -= seedWpl;
  maskLine -= maskWpl;
  prevLine = seedLine;

  // Bottom to top.
  for (int y = h - 1; y >= 0; --y) {
    uint32_t prevWord = 0;

    // Make sure offscreen bits area 0.
    seedLine[lastWordIdx] &= lastWordMask;
    // Right to left.
    for (int i = lastWordIdx; i >= 0; --i) {
      const uint32_t mask = maskLine[i];
      uint32_t word = prevWord >> 31;
      word |= seedLine[i] | prevLine[i];
      word &= mask;
      word = fillWordHorizontally(word, mask);
      seedLine[i] = word;
      prevWord = word;
    }
    // If we don't do this, prevLine[lastWordIdx] on the next
    // iteration may contain garbage in the off-screen area.
    // That garbage can easily leak back.
    // Fortunately, garbage can't spread through prevWord,
    // as only 1 bit is used from it, which can't be garbage.
    seedLine[lastWordIdx] &= lastWordMask;

    prevLine = seedLine;
    seedLine -= seedWpl;
    maskLine -= maskWpl;
  }
}  // seedFill4Iteration

void seedFill8Iteration(BinaryImage& seed, const BinaryImage& mask) {
  const int w = seed.width();
  const int h = seed.height();

  const int seedWpl = seed.wordsPerLine();
  const int maskWpl = mask.wordsPerLine();
  const int lastWordIdx = (w - 1) >> 5;
  const uint32_t lastWordMask = ~uint32_t(0) << (((lastWordIdx + 1) << 5) - w);

  uint32_t* seedLine = seed.data();
  const uint32_t* maskLine = mask.data();
  const uint32_t* prevLine = seedLine;

  // Note: we start with prevLine == seedLine, but in this case
  // prevLine[i + 1] won't be clipped by its mask when we use it to
  // update seedLine[i].  The wrong value may propagate further from
  // there, so clipping we do on the anti-raster pass won't help.
  // That's why we clip the first line here.
  for (int i = 0; i <= lastWordIdx; ++i) {
    seedLine[i] &= maskLine[i];
  }

  // Top to bottom.
  for (int y = 0; y < h; ++y) {
    uint32_t prevWord = 0;

    // Make sure offscreen bits area 0.
    seedLine[lastWordIdx] &= lastWordMask;
    // Left to right (except the last word).
    int i = 0;
    for (; i < lastWordIdx; ++i) {
      const uint32_t mask = maskLine[i];
      uint32_t word = prevLine[i];
      word |= (word << 1) | (word >> 1);
      word |= seedLine[i];
      word |= prevLine[i + 1] >> 31;
      word |= prevWord << 31;
      word &= mask;
      word = fillWordHorizontally(word, mask);
      seedLine[i] = word;
      prevWord = word;
    }
    // Last word.
    const uint32_t mask = maskLine[i] & lastWordMask;
    uint32_t word = prevLine[i];
    word |= (word << 1) | (word >> 1);
    word |= seedLine[i];
    word |= prevWord << 31;
    word &= mask;
    word = fillWordHorizontally(word, mask);
    seedLine[i] = word;

    prevLine = seedLine;
    seedLine += seedWpl;
    maskLine += maskWpl;
  }

  seedLine -= seedWpl;
  maskLine -= maskWpl;
  prevLine = seedLine;

  // Bottom to top.
  for (int y = h - 1; y >= 0; --y) {
    uint32_t prevWord = 0;

    // Make sure offscreen bits area 0.
    seedLine[lastWordIdx] &= lastWordMask;
    // Right to left (except the last word).
    int i = lastWordIdx;
    for (; i > 0; --i) {
      const uint32_t mask = maskLine[i];
      uint32_t word = prevLine[i];
      word |= (word << 1) | (word >> 1);
      word |= seedLine[i];
      word |= prevLine[i - 1] << 31;
      word |= prevWord >> 31;
      word &= mask;
      word = fillWordHorizontally(word, mask);
      seedLine[i] = word;
      prevWord = word;
    }

    // Last word.
    const uint32_t mask = maskLine[i];
    uint32_t word = prevLine[i];
    word |= (word << 1) | (word >> 1);
    word |= seedLine[i];
    word |= prevWord >> 31;
    word &= mask;
    word = fillWordHorizontally(word, mask);
    seedLine[i] = word;
    // If we don't do this, prevLine[lastWordIdx] on the next
    // iteration may contain garbage in the off-screen area.
    // That garbage can easily leak back.
    // Fortunately, garbage can't spread through prevWord,
    // as only 1 bit is used from it, which can't be garbage.
    seedLine[lastWordIdx] &= lastWordMask;

    prevLine = seedLine;
    seedLine -= seedWpl;
    maskLine -= maskWpl;
  }
}  // seedFill8Iteration

inline uint8_t lightest(uint8_t lhs, uint8_t rhs) {
  return lhs > rhs ? lhs : rhs;
}

inline uint8_t darkest(uint8_t lhs, uint8_t rhs) {
  return lhs < rhs ? lhs : rhs;
}

inline bool darkerThan(uint8_t lhs, uint8_t rhs) {
  return lhs < rhs;
}

void seedFillGrayHorLine(uint8_t* seed, const uint8_t* mask, const int lineLen) {
  assert(lineLen > 0);

  *seed = lightest(*seed, *mask);

  for (int i = 1; i < lineLen; ++i) {
    ++seed;
    ++mask;
    *seed = lightest(*mask, darkest(*seed, seed[-1]));
  }

  for (int i = 1; i < lineLen; ++i) {
    --seed;
    --mask;
    *seed = lightest(*mask, darkest(*seed, seed[1]));
  }
}

void seedFillGrayVertLine(uint8_t* seed,
                          const int seedStride,
                          const uint8_t* mask,
                          const int maskStride,
                          const int lineLen) {
  assert(lineLen > 0);

  *seed = lightest(*seed, *mask);

  for (int i = 1; i < lineLen; ++i) {
    seed += seedStride;
    mask += maskStride;
    *seed = lightest(*mask, darkest(*seed, seed[-seedStride]));
  }

  for (int i = 1; i < lineLen; ++i) {
    seed -= seedStride;
    mask -= maskStride;
    *seed = lightest(*mask, darkest(*seed, seed[seedStride]));
  }
}

/**
 * \return non-zero if more iterations are required, zero otherwise.
 */
uint8_t seedFillGray4SlowIteration(GrayImage& seed, const GrayImage& mask) {
  const int w = seed.width();
  const int h = seed.height();

  uint8_t* seedLine = seed.data();
  const uint8_t* maskLine = mask.data();
  const uint8_t* prevLine = seedLine;

  const int seedStride = seed.stride();
  const int maskStride = mask.stride();

  uint8_t modified = 0;

  // Top to bottom.
  for (int y = 0; y < h; ++y) {
    uint8_t prevPixel = 0xff;
    // Left to right.
    for (int x = 0; x < w; ++x) {
      const uint8_t pixel = lightest(maskLine[x], darkest(prevPixel, darkest(seedLine[x], prevLine[x])));
      modified |= seedLine[x] ^ pixel;
      seedLine[x] = pixel;
      prevPixel = pixel;
    }

    prevLine = seedLine;
    seedLine += seedStride;
    maskLine += maskStride;
  }

  seedLine -= seedStride;
  maskLine -= maskStride;
  prevLine = seedLine;

  // Bottom to top.
  for (int y = h - 1; y >= 0; --y) {
    uint8_t prevPixel = 0xff;
    // Right to left.
    for (int x = w - 1; x >= 0; --x) {
      const uint8_t pixel = lightest(maskLine[x], darkest(prevPixel, darkest(seedLine[x], prevLine[x])));
      modified |= seedLine[x] ^ pixel;
      seedLine[x] = pixel;
      prevPixel = pixel;
    }

    prevLine = seedLine;
    seedLine -= seedStride;
    maskLine -= maskStride;
  }

  return modified;
}  // seedFillGray4SlowIteration

/**
 * \return non-zero if more iterations are required, zero otherwise.
 */
uint8_t seedFillGray8SlowIteration(GrayImage& seed, const GrayImage& mask) {
  const int w = seed.width();
  const int h = seed.height();

  uint8_t* seedLine = seed.data();
  const uint8_t* maskLine = mask.data();
  const uint8_t* prevLine = seedLine;

  const int seedStride = seed.stride();
  const int maskStride = mask.stride();

  uint8_t modified = 0;

  // Some code below doesn't handle such cases.
  if (w == 1) {
    seedFillGrayVertLine(seedLine, seedStride, maskLine, maskStride, h);

    return 0;
  } else if (h == 1) {
    seedFillGrayHorLine(seedLine, maskLine, w);

    return 0;
  }

  // The prevLine[x + 1] below actually refers to seedLine[x + 1]
  // for the first line in raster order.  When working with seedLine[x],
  // seedLine[x + 1] would not yet be clipped by its mask.  So, we
  // have to do it now.
  for (int x = 0; x < w; ++x) {
    seedLine[x] = lightest(seedLine[x], maskLine[x]);
  }

  // Top to bottom.
  for (int y = 0; y < h; ++y) {
    int x = 0;
    // Leftmost pixel.
    uint8_t pixel = lightest(maskLine[x], darkest(seedLine[x], darkest(prevLine[x], prevLine[x + 1])));
    modified |= seedLine[x] ^ pixel;
    seedLine[x] = pixel;
    // Left to right.
    while (++x < w - 1) {
      pixel = lightest(maskLine[x],
                       darkest(darkest(darkest(seedLine[x], seedLine[x - 1]), darkest(prevLine[x], prevLine[x - 1])),
                               prevLine[x + 1]));
      modified |= seedLine[x] ^ pixel;
      seedLine[x] = pixel;
    }
    // Rightmost pixel.
    pixel
        = lightest(maskLine[x], darkest(darkest(seedLine[x], seedLine[x - 1]), darkest(prevLine[x], prevLine[x - 1])));
    modified |= seedLine[x] ^ pixel;
    seedLine[x] = pixel;

    prevLine = seedLine;
    seedLine += seedStride;
    maskLine += maskStride;
  }

  seedLine -= seedStride;
  maskLine -= maskStride;
  prevLine = seedLine;

  // Bottom to top.
  for (int y = h - 1; y >= 0; --y) {
    int x = w - 1;
    // Rightmost pixel.
    uint8_t pixel = lightest(maskLine[x], darkest(seedLine[x], darkest(prevLine[x], prevLine[x - 1])));
    modified |= seedLine[x] ^ pixel;
    seedLine[x] = pixel;
    // Right to left.
    while (--x > 0) {
      pixel = lightest(maskLine[x],
                       darkest(darkest(darkest(seedLine[x], seedLine[x + 1]), darkest(prevLine[x], prevLine[x + 1])),
                               prevLine[x - 1]));
      modified |= seedLine[x] ^ pixel;
      seedLine[x] = pixel;
    }
    // Leftmost pixel.
    pixel
        = lightest(maskLine[x], darkest(darkest(seedLine[x], seedLine[x + 1]), darkest(prevLine[x], prevLine[x + 1])));
    modified |= seedLine[x] ^ pixel;
    seedLine[x] = pixel;

    prevLine = seedLine;
    seedLine -= seedStride;
    maskLine -= maskStride;
  }

  return modified;
}  // seedFillGray8SlowIteration
}  // namespace

BinaryImage seedFill(const BinaryImage& seed, const BinaryImage& mask, const Connectivity connectivity) {
  if (seed.size() != mask.size()) {
    throw std::invalid_argument("seedFill: seed and mask have different sizes");
  }

  BinaryImage prev;
  BinaryImage img(seed);

  do {
    prev = img;
    if (connectivity == CONN4) {
      seedFill4Iteration(img, mask);
    } else {
      seedFill8Iteration(img, mask);
    }
  } while (img != prev);

  return img;
}

GrayImage seedFillGray(const GrayImage& seed, const GrayImage& mask, const Connectivity connectivity) {
  GrayImage result(seed);
  seedFillGrayInPlace(result, mask, connectivity);

  return result;
}

void seedFillGrayInPlace(GrayImage& seed, const GrayImage& mask, const Connectivity connectivity) {
  if (seed.size() != mask.size()) {
    throw std::invalid_argument("seedFillGrayInPlace: seed and mask have different sizes");
  }

  if (seed.isNull()) {
    return;
  }

  seedFillGenericInPlace(&darkest, &lightest, connectivity, seed.data(), seed.stride(), seed.size(), mask.data(),
                         mask.stride());
}

GrayImage seedFillGraySlow(const GrayImage& seed, const GrayImage& mask, const Connectivity connectivity) {
  GrayImage img(seed);

  if (connectivity == CONN4) {
    while (seedFillGray4SlowIteration(img, mask)) {
      // Continue until done.
    }
  } else {
    while (seedFillGray8SlowIteration(img, mask)) {
      // Continue until done.
    }
  }

  return img;
}
}  // namespace imageproc