/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2007-2008  Joseph Artsimovich <joseph_a@mail.ru>

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

#include "SlicedHistogram.h"
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
  const int first_word_idx = area.left() >> 5;
  const int last_word_idx = area.right() >> 5;  // area.right() is within area
  const uint32_t first_word_mask = ~uint32_t(0) >> (area.left() & 31);
  const int last_word_unused_bits = (last_word_idx << 5) + 31 - area.right();
  const uint32_t last_word_mask = ~uint32_t(0) << last_word_unused_bits;
  const uint32_t* line = image.data() + top * wpl;

  if (first_word_idx == last_word_idx) {
    const uint32_t mask = first_word_mask & last_word_mask;
    for (int y = top; y <= bottom; ++y, line += wpl) {
      const int count = countNonZeroBits(line[first_word_idx] & mask);
      m_data.push_back(count);
    }
  } else {
    for (int y = top; y <= bottom; ++y, line += wpl) {
      int idx = first_word_idx;
      int count = countNonZeroBits(line[idx] & first_word_mask);
      for (++idx; idx != last_word_idx; ++idx) {
        count += countNonZeroBits(line[idx]);
      }
      count += countNonZeroBits(line[idx] & last_word_mask);
      m_data.push_back(count);
    }
  }
}  // SlicedHistogram::processHorizontalLines

void SlicedHistogram::processVerticalLines(const BinaryImage& image, const QRect& area) {
  m_data.reserve(area.width());

  const int right = area.right();
  const int height = area.height();
  const int wpl = image.wordsPerLine();
  const uint32_t* const top_line = image.data() + area.top() * wpl;

  for (int x = area.left(); x <= right; ++x) {
    const uint32_t* pword = top_line + (x >> 5);
    const int least_significant_zeroes = 31 - (x & 31);
    int count = 0;
    for (int i = 0; i < height; ++i, pword += wpl) {
      count += (*pword >> least_significant_zeroes) & 1;
    }
    m_data.push_back(count);
  }
}
}  // namespace imageproc