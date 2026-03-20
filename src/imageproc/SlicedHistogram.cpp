// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "SlicedHistogram.h"

#include <stdexcept>

#include "BinaryImage.h"
#include "BitOps.h"

namespace imageproc {
SlicedHistogram::SlicedHistogram() = default;

SlicedHistogram::SlicedHistogram(const BinaryImage& image, const Type type) {
  switch (type) {
    case ROWS:
      processHorizontalLines(image, image.rect());
      break;
    case COLS:
      processVerticalLines(image, image.rect());
      break;
  }
}

SlicedHistogram::SlicedHistogram(const BinaryImage& image, const QRect& area, const Type type) {
  if (!image.rect().contains(area)) {
    throw std::invalid_argument("SlicedHistogram: area exceeds the image");
  }

  switch (type) {
    case ROWS:
      processHorizontalLines(image, area);
      break;
    case COLS:
      processVerticalLines(image, area);
      break;
  }
}

void SlicedHistogram::processHorizontalLines(const BinaryImage& image, const QRect& area) {
  m_data.reserve(area.height());

  const int top = area.top();
  const int bottom = area.bottom();
  const int wpl = image.wordsPerLine();
  const int firstWordIdx = area.left() >> 5;
  const int lastWordIdx = area.right() >> 5;  // area.right() is within area
  const uint32_t firstWordMask = ~uint32_t(0) >> (area.left() & 31);
  const int lastWordUnusedBits = (lastWordIdx << 5) + 31 - area.right();
  const uint32_t lastWordMask = ~uint32_t(0) << lastWordUnusedBits;
  const uint32_t* line = image.data() + top * wpl;

  if (firstWordIdx == lastWordIdx) {
    const uint32_t mask = firstWordMask & lastWordMask;
    for (int y = top; y <= bottom; ++y, line += wpl) {
      const int count = countNonZeroBits(line[firstWordIdx] & mask);
      m_data.push_back(count);
    }
  } else {
    for (int y = top; y <= bottom; ++y, line += wpl) {
      int idx = firstWordIdx;
      int count = countNonZeroBits(line[idx] & firstWordMask);
      for (++idx; idx != lastWordIdx; ++idx) {
        count += countNonZeroBits(line[idx]);
      }
      count += countNonZeroBits(line[idx] & lastWordMask);
      m_data.push_back(count);
    }
  }
}  // SlicedHistogram::processHorizontalLines

void SlicedHistogram::processVerticalLines(const BinaryImage& image, const QRect& area) {
  m_data.reserve(area.width());

  const int right = area.right();
  const int height = area.height();
  const int wpl = image.wordsPerLine();
  const uint32_t* const topLine = image.data() + area.top() * wpl;

  for (int x = area.left(); x <= right; ++x) {
    const uint32_t* pword = topLine + (x >> 5);
    const int leastSignificantZeroes = 31 - (x & 31);
    int count = 0;
    for (int i = 0; i < height; ++i, pword += wpl) {
      count += (*pword >> leastSignificantZeroes) & 1;
    }
    m_data.push_back(count);
  }
}
}  // namespace imageproc
