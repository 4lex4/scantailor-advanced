// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

/* The code is based on Paul Heckbert's stack-based seed fill algorithm
 * from "Graphic Gems", ed. Andrew Glassner, Academic Press, 1990.
 * This version is optimized to elliminate all multiplications. */

#include "ConnCompEraser.h"
#include <cassert>
#include "BitOps.h"

namespace imageproc {
struct ConnCompEraser::BBox {
  int xmin;
  int xmax;
  int ymin;
  int ymax;

  BBox(int x, int y) : xmin(x), xmax(x), ymin(y), ymax(y) {}

  int width() const { return xmax - xmin + 1; }

  int height() const { return ymax - ymin + 1; }
};


inline uint32_t ConnCompEraser::getBit(const uint32_t* const line, const int x) {
  const uint32_t mask = (uint32_t(1) << 31) >> (x & 31);

  return line[x >> 5] & mask;
}

inline void ConnCompEraser::clearBit(uint32_t* const line, const int x) {
  const uint32_t mask = (uint32_t(1) << 31) >> (x & 31);
  line[x >> 5] &= ~mask;
}

ConnCompEraser::ConnCompEraser(const BinaryImage& image, Connectivity conn)
    : m_image(image),
      m_line(nullptr),
      m_width(m_image.width()),
      m_height(m_image.height()),
      m_wpl(m_image.wordsPerLine()),
      m_connectivity(conn),
      m_x(0),
      m_y(0) {
  // By initializing m_line with 0 instead of m_image.data(),
  // we avoid copy-on-write, provided that the caller used image.release().
}

ConnComp ConnCompEraser::nextConnComp() {
  if (!moveToNextBlackPixel()) {
    return ConnComp();
  }

  if (m_connectivity == CONN4) {
    return eraseConnComp4();
  } else {
    return eraseConnComp8();
  }
}

void ConnCompEraser::pushSegSameDir(const Segment& seg, int xleft, int xright, BBox& bbox) {
  bbox.xmin = std::min(bbox.xmin, xleft);
  bbox.xmax = std::max(bbox.xmax, xright);
  bbox.ymin = std::min(bbox.ymin, seg.y);
  bbox.ymax = std::max(bbox.ymax, seg.y);

  const int newDy = seg.dy;
  const int newDyWpl = seg.dyWpl;
  const int newY = seg.y + newDy;
  if ((newY >= 0) && (newY < m_height)) {
    Segment new_seg{};
    new_seg.line = seg.line + newDyWpl;
    new_seg.xleft = xleft;
    new_seg.xright = xright;
    new_seg.y = newY;
    new_seg.dy = newDy;
    new_seg.dyWpl = newDyWpl;
    m_segStack.push(new_seg);
  }
}

void ConnCompEraser::pushSegInvDir(const Segment& seg, int xleft, int xright, BBox& bbox) {
  bbox.xmin = std::min(bbox.xmin, xleft);
  bbox.xmax = std::max(bbox.xmax, xright);
  bbox.ymin = std::min(bbox.ymin, seg.y);
  bbox.ymax = std::max(bbox.ymax, seg.y);

  const int newDy = -seg.dy;
  const int newDyWpl = -seg.dyWpl;
  const int newY = seg.y + newDy;
  if ((newY >= 0) && (newY < m_height)) {
    Segment new_seg{};
    new_seg.line = seg.line + newDyWpl;
    new_seg.xleft = xleft;
    new_seg.xright = xright;
    new_seg.y = newY;
    new_seg.dy = newDy;
    new_seg.dyWpl = newDyWpl;
    m_segStack.push(new_seg);
  }
}

void ConnCompEraser::pushInitialSegments() {
  assert(m_x >= 0 && m_x < m_width);
  assert(m_y >= 0 && m_y < m_height);

  if (m_y + 1 < m_height) {
    Segment seg1{};
    seg1.line = m_line + m_wpl;
    seg1.xleft = m_x;
    seg1.xright = m_x;
    seg1.y = m_y + 1;
    seg1.dy = 1;
    seg1.dyWpl = m_wpl;
    m_segStack.push(seg1);
  }

  Segment seg2{};
  seg2.line = m_line;
  seg2.xleft = m_x;
  seg2.xright = m_x;
  seg2.y = m_y;
  seg2.dy = -1;
  seg2.dyWpl = -m_wpl;
  m_segStack.push(seg2);
}

bool ConnCompEraser::moveToNextBlackPixel() {
  if (m_image.isNull()) {
    return false;
  }

  if (!m_line) {
    // By initializing m_line with 0 instead of m_image.data(),
    // we allow the caller to delete his copy of the image
    // to avoid copy-on-write.
    // We could also try to avoid copy-on-write in the case of
    // a completely white image, but I don't think it's worth it.
    m_line = m_image.data();
  }

  uint32_t* line = m_line;
  const uint32_t* pword = line + (m_x >> 5);

  // Stop word is a last word in line that holds data.
  const int lastBitIdx = m_width - 1;
  const uint32_t* pStopWord = line + (lastBitIdx >> 5);
  const uint32_t stopWordMask = ~uint32_t(0) << (31 - (lastBitIdx & 31));

  uint32_t word = *pword;
  if (pword == pStopWord) {
    word &= stopWordMask;
  }
  word <<= (m_x & 31);
  if (word) {
    const int shift = countMostSignificantZeroes(word);
    m_x += shift;
    assert(m_x < m_width);

    return true;
  }

  int y = m_y;
  if (pword != pStopWord) {
    ++pword;
  } else {
    ++y;
    line += m_wpl;
    pStopWord += m_wpl;
    pword = line;
  }

  for (; y < m_height; ++y) {
    for (; pword != pStopWord; ++pword) {
      word = *pword;
      if (word) {
        const int shift = countMostSignificantZeroes(word);
        m_x = static_cast<int>(((pword - line) << 5) + shift);
        assert(m_x < m_width);
        m_y = y;
        m_line = line;

        return true;
      }
    }
    // Handle the stop word (some bits need to be ignored).
    assert(pword == pStopWord);
    word = *pword & stopWordMask;
    if (word) {
      const int shift = countMostSignificantZeroes(word);
      m_x = static_cast<int>(((pword - line) << 5) + shift);
      assert(m_x < m_width);
      m_y = y;
      m_line = line;

      return true;
    }

    line += m_wpl;
    pStopWord += m_wpl;
    pword = line;
  }

  return false;
}  // ConnCompEraser::moveToNextBlackPixel

ConnComp ConnCompEraser::eraseConnComp4() {
  pushInitialSegments();

  BBox bbox(m_x, m_y);
  int pixCount = 0;

  while (!m_segStack.empty()) {
    // Pop a segment off the stack.
    const Segment seg(m_segStack.top());
    m_segStack.pop();

    const int xmax = std::min(seg.xright, m_width - 1);

    int x = seg.xleft;
    for (; x >= 0 && getBit(seg.line, x); --x) {
      clearBit(seg.line, x);
      ++pixCount;
    }

    int xstart = x + 1;

    if (x >= seg.xleft) {
      // Pixel at seg.xleft was off and was not cleared.
      goto skip;
    }

    if (xstart < seg.xleft - 1) {
      // Leak on left.
      pushSegInvDir(seg, xstart, seg.xleft - 1, bbox);
    }

    x = seg.xleft + 1;

    do {
      for (; x < m_width && getBit(seg.line, x); ++x) {
        clearBit(seg.line, x);
        ++pixCount;
      }
      pushSegSameDir(seg, xstart, x - 1, bbox);
      if (x > seg.xright + 1) {
        // Leak on right.
        pushSegInvDir(seg, seg.xright + 1, x - 1, bbox);
      }

    skip:
      for (++x; x <= xmax && !getBit(seg.line, x); ++x) {
        // Skip white pixels.
      }
      xstart = x;
    } while (x <= xmax);
  }

  QRect rect(bbox.xmin, bbox.ymin, bbox.width(), bbox.height());

  return ConnComp(QPoint(m_x, m_y), rect, pixCount);
}  // ConnCompEraser::eraseConnComp4

ConnComp ConnCompEraser::eraseConnComp8() {
  pushInitialSegments();

  BBox bbox(m_x, m_y);
  int pixCount = 0;

  while (!m_segStack.empty()) {
    // Pop a segment off the stack.
    const Segment seg(m_segStack.top());
    m_segStack.pop();

    const int xmax = std::min(seg.xright + 1, m_width - 1);

    int x = seg.xleft - 1;
    for (; x >= 0 && getBit(seg.line, x); --x) {
      clearBit(seg.line, x);
      ++pixCount;
    }

    int xstart = x + 1;

    if (x >= seg.xleft - 1) {
      // Pixel at seg.xleft - 1 was off and was not cleared.
      goto skip;
    }

    if (xstart < seg.xleft) {
      // Leak on left.
      pushSegInvDir(seg, xstart, seg.xleft - 1, bbox);
    }

    x = seg.xleft;
    do {
      for (; x < m_width && getBit(seg.line, x); ++x) {
        clearBit(seg.line, x);
        ++pixCount;
      }
      pushSegSameDir(seg, xstart, x - 1, bbox);
      if (x > seg.xright) {
        // Leak on right.
        pushSegInvDir(seg, seg.xright + 1, x - 1, bbox);
      }

    skip:
      for (++x; x <= xmax && !getBit(seg.line, x); ++x) {
        // Skip white pixels.
      }
      xstart = x;
    } while (x <= xmax);
  }

  QRect rect(bbox.xmin, bbox.ymin, bbox.width(), bbox.height());

  return ConnComp(QPoint(m_x, m_y), rect, pixCount);
}  // ConnCompEraser::eraseConnComp8
}  // namespace imageproc