// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "BinaryImage.h"

#include <QAtomicInt>
#include <QImage>
#include <QRect>
#include <QtEndian>
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <new>
#include <stdexcept>

#include "BitOps.h"
#include "ByteOrder.h"

namespace imageproc {
class BinaryImage::SharedData {
 private:
  /**
   * Resolves the ambiguity of:
   * \code
   * void SharedData::operator delete(void*, size_t);
   * \endcode
   * Which may be interpreted as both a placement and non-placement delete.
   */
  struct NumWords {
    size_t numWords;

    explicit NumWords(size_t numWords) : numWords(numWords) {}
  };

 public:
  static SharedData* create(size_t numWords) { return new (NumWords(numWords)) SharedData(); }

  uint32_t* data() { return m_data; }

  const uint32_t* data() const { return m_data; }

  bool isShared() const { return m_counter.fetchAndAddRelaxed(0) > 1; }

  void ref() const { m_counter.ref(); }

  void unref() const;

  static void* operator new(size_t size, NumWords numWords);

  static void operator delete(void* addr, NumWords numWords);

 private:
  SharedData() : m_counter(1) {}

  SharedData& operator=(const SharedData&) = delete;  // forbidden

  mutable QAtomicInt m_counter;
  uint32_t m_data[1]{};  // more data follows
};


BinaryImage::BinaryImage() : m_data(nullptr), m_width(0), m_height(0), m_wpl(0) {}

BinaryImage::BinaryImage(const int width, const int height)
    : m_width(width), m_height(height), m_wpl((width + 31) / 32) {
  if ((m_width > 0) && (m_height > 0)) {
    m_data = SharedData::create(m_height * m_wpl);
  } else {
    throw std::invalid_argument("BinaryImage dimensions are wrong");
  }
}

BinaryImage::BinaryImage(const QSize size)
    : m_width(size.width()), m_height(size.height()), m_wpl((size.width() + 31) / 32) {
  if ((m_width > 0) && (m_height > 0)) {
    m_data = SharedData::create(m_height * m_wpl);
  } else {
    throw std::invalid_argument("BinaryImage dimensions are wrong");
  }
}

BinaryImage::BinaryImage(const int width, const int height, const BWColor color)
    : m_width(width), m_height(height), m_wpl((width + 31) / 32) {
  if ((m_width > 0) && (m_height > 0)) {
    m_data = SharedData::create(m_height * m_wpl);
  } else {
    throw std::invalid_argument("BinaryImage dimensions are wrong");
  }
  fill(color);
}

BinaryImage::BinaryImage(const QSize size, const BWColor color)
    : m_width(size.width()), m_height(size.height()), m_wpl((size.width() + 31) / 32) {
  if ((m_width > 0) && (m_height > 0)) {
    m_data = SharedData::create(m_height * m_wpl);
  } else {
    throw std::invalid_argument("BinaryImage dimensions are wrong");
  }
  fill(color);
}

BinaryImage::BinaryImage(const int width, const int height, SharedData* const data)
    : m_data(data), m_width(width), m_height(height), m_wpl((width + 31) / 32) {}

BinaryImage::BinaryImage(const BinaryImage& other)
    : m_data(other.m_data), m_width(other.m_width), m_height(other.m_height), m_wpl(other.m_wpl) {
  if (m_data) {
    m_data->ref();
  }
}

BinaryImage::BinaryImage(const QImage& image, const BinaryThreshold threshold)
    : m_data(nullptr), m_width(0), m_height(0), m_wpl(0) {
  const QRect imageRect(image.rect());

  switch (image.format()) {
    case QImage::Format_Invalid:
      break;
    case QImage::Format_Mono:
      *this = fromMono(image);
      break;
    case QImage::Format_MonoLSB:
      *this = fromMonoLSB(image);
      break;
    case QImage::Format_Indexed8:
      *this = fromIndexed8(image, imageRect, threshold);
      break;
    case QImage::Format_RGB32:
    case QImage::Format_ARGB32:
      *this = fromRgb32(image, imageRect, threshold);
      break;
    case QImage::Format_ARGB32_Premultiplied:
      *this = fromArgb32Premultiplied(image, imageRect, threshold);
      break;
    case QImage::Format_RGB16:
      *this = fromRgb16(image, imageRect, threshold);
      break;
    default:
      throw std::runtime_error("Unsupported QImage format");
  }
}

BinaryImage::BinaryImage(const QImage& image, const QRect& rect, const BinaryThreshold threshold)
    : m_data(nullptr), m_width(0), m_height(0), m_wpl(0) {
  if (rect.isEmpty()) {
    return;
  } else if (rect.intersected(image.rect()) != rect) {
    throw std::invalid_argument("BinaryImage: rect exceedes the QImage");
  }

  switch (image.format()) {
    case QImage::Format_Invalid:
      break;
    case QImage::Format_Mono:
      *this = fromMono(image, rect);
      break;
    case QImage::Format_MonoLSB:
      *this = fromMonoLSB(image, rect);
      break;
    case QImage::Format_Indexed8:
      *this = fromIndexed8(image, rect, threshold);
      break;
    case QImage::Format_RGB32:
    case QImage::Format_ARGB32:
      *this = fromRgb32(image, rect, threshold);
      break;
    case QImage::Format_ARGB32_Premultiplied:
      *this = fromArgb32Premultiplied(image, rect, threshold);
      break;
    case QImage::Format_RGB16:
      *this = fromRgb16(image, rect, threshold);
      break;
    default:
      throw std::runtime_error("BinaryImage: Unsupported QImage format");
  }
}

BinaryImage::~BinaryImage() {
  if (m_data) {
    m_data->unref();
  }
}

BinaryImage& BinaryImage::operator=(const BinaryImage& other) {
  BinaryImage(other).swap(*this);
  return *this;
}

void BinaryImage::swap(BinaryImage& other) {
  std::swap(m_data, other.m_data);
  std::swap(m_width, other.m_width);
  std::swap(m_height, other.m_height);
  std::swap(m_wpl, other.m_wpl);
}

void BinaryImage::invert() {
  if (isNull()) {
    return;
  }

  const size_t numWords = m_height * m_wpl;

  assert(m_data);
  if (!m_data->isShared()) {
    // In-place operation
    uint32_t* data = this->data();
    for (size_t i = 0; i < numWords; ++i, ++data) {
      *data = ~*data;
    }
  } else {
    SharedData* newData = SharedData::create(numWords);

    const uint32_t* srcData = m_data->data();
    uint32_t* dstData = newData->data();
    for (size_t i = 0; i < numWords; ++i, ++srcData, ++dstData) {
      *dstData = ~*srcData;
    }

    m_data->unref();
    m_data = newData;
  }
}

BinaryImage BinaryImage::inverted() const {
  if (isNull()) {
    return BinaryImage();
  }

  const size_t numWords = m_height * m_wpl;
  SharedData* newData = SharedData::create(numWords);

  const uint32_t* srcData = m_data->data();
  uint32_t* dstData = newData->data();
  for (size_t i = 0; i < numWords; ++i, ++srcData, ++dstData) {
    *dstData = ~*srcData;
  }
  return BinaryImage(m_width, m_height, newData);
}

void BinaryImage::fill(const BWColor color) {
  if (isNull()) {
    throw std::logic_error("Attempt to fill a null BinaryImage!");
  }

  const int pattern = (color == BLACK) ? ~0 : 0;
  memset(data(), pattern, m_height * m_wpl * 4);
}

void BinaryImage::fill(const QRect& rect, const BWColor color) {
  if (rect.isEmpty()) {
    return;
  }

  fillRectImpl(data(), rect.intersected(this->rect()), color);
}

void BinaryImage::fillExcept(const QRect& rect, const BWColor color) {
  if (isNull()) {
    throw std::logic_error("Attempt to fill a null BinaryImage!");
  }

  if (rect.contains(this->rect())) {
    return;
  }

  const QRect boundedRect(rect.intersected(this->rect()));
  if (boundedRect.isEmpty()) {
    fill(color);
    return;
  }

  const int pattern = (color == BLACK) ? ~0 : 0;
  uint32_t* const data = this->data();  // this will call copyIfShared()
  if (boundedRect.top() > 0) {
    memset(data, pattern, boundedRect.top() * m_wpl * 4);
  }

  int yTop = boundedRect.top();
  if (boundedRect.left() > 0) {
    const QRect leftRect(0, yTop, boundedRect.left(), boundedRect.height());
    fillRectImpl(data, leftRect, color);
  }

  const int xLeft = boundedRect.left() + boundedRect.width();
  if (xLeft < m_width) {
    const QRect rightRect(xLeft, yTop, m_width - xLeft, boundedRect.height());
    fillRectImpl(data, rightRect, color);
  }

  yTop = boundedRect.top() + boundedRect.height();
  if (yTop < m_height) {
    memset(data + yTop * m_wpl, pattern, (m_height - yTop) * m_wpl * 4);
  }
}  // BinaryImage::fillExcept

void BinaryImage::fillFrame(const QRect& outerRect, const QRect& innerRect, const BWColor color) {
  if (isNull()) {
    throw std::logic_error("Attempt to fill a null BinaryImage!");
  }

  const QRect boundedOuterRect(outerRect.intersected(this->rect()));
  const QRect boundedInnerRect(innerRect.intersected(boundedOuterRect));
  if (boundedInnerRect == boundedOuterRect) {
    return;
  } else if (boundedInnerRect.isEmpty()) {
    fill(boundedOuterRect, color);
    return;
  }

  uint32_t* const data = this->data();

  QRect topRect(boundedOuterRect);
  topRect.setBottom(boundedInnerRect.top() - 1);
  if (topRect.height() != 0) {
    fillRectImpl(data, topRect, color);
  }

  QRect leftRect(boundedInnerRect);
  leftRect.setLeft(boundedOuterRect.left());
  leftRect.setRight(boundedInnerRect.left() - 1);
  if (leftRect.width() != 0) {
    fillRectImpl(data, leftRect, color);
  }

  QRect rightRect(boundedInnerRect);
  rightRect.setRight(boundedOuterRect.right());
  rightRect.setLeft(boundedInnerRect.right() + 1);
  if (rightRect.width() != 0) {
    fillRectImpl(data, rightRect, color);
  }

  QRect bottomRect(boundedOuterRect);
  bottomRect.setTop(boundedInnerRect.bottom() + 1);
  if (bottomRect.height() != 0) {
    fillRectImpl(data, bottomRect, color);
  }
}  // BinaryImage::fillFrame

int BinaryImage::countBlackPixels() const {
  return countBlackPixels(rect());
}

int BinaryImage::countWhitePixels() const {
  return countWhitePixels(rect());
}

int BinaryImage::countBlackPixels(const QRect& rect) const {
  const QRect r(rect.intersected(this->rect()));
  if (r.isEmpty()) {
    return 0;
  }

  const int top = r.top();
  const int bottom = r.bottom();
  const int firstWordIdx = r.left() >> 5;
  const int lastWordIdx = r.right() >> 5;  // r.right() is within rect
  const uint32_t firstWordMask = ~uint32_t(0) >> (r.left() & 31);
  const int lastWordUnusedBits = (lastWordIdx << 5) + 31 - r.right();
  const uint32_t lastWordMask = ~uint32_t(0) << lastWordUnusedBits;
  const uint32_t* line = data() + top * m_wpl;

  int count = 0;

  if (firstWordIdx == lastWordIdx) {
    if (r.width() == 1) {
      for (int y = top; y <= bottom; ++y, line += m_wpl) {
        count += (line[firstWordIdx] >> lastWordUnusedBits) & 1;
      }
    } else {
      const uint32_t mask = firstWordMask & lastWordMask;
      for (int y = top; y <= bottom; ++y, line += m_wpl) {
        count += countNonZeroBits(line[firstWordIdx] & mask);
      }
    }
  } else {
    for (int y = top; y <= bottom; ++y, line += m_wpl) {
      int idx = firstWordIdx;
      count += countNonZeroBits(line[idx] & firstWordMask);
      for (++idx; idx != lastWordIdx; ++idx) {
        count += countNonZeroBits(line[idx]);
      }
      count += countNonZeroBits(line[idx] & lastWordMask);
    }
  }
  return count;
}  // BinaryImage::countBlackPixels

int BinaryImage::countWhitePixels(const QRect& rect) const {
  const QRect r(rect.intersected(this->rect()));
  if (r.isEmpty()) {
    return 0;
  }
  return r.width() * r.height() - countBlackPixels(r);
}

QRect BinaryImage::contentBoundingBox(const BWColor contentColor) const {
  if (isNull()) {
    return QRect();
  }

  const int w = m_width;
  const int h = m_height;
  const int wpl = m_wpl;
  const int lastWordIdx = (w - 1) >> 5;
  const int lastWordBits = w - (lastWordIdx << 5);
  const int lastWordUnusedBits = 32 - lastWordBits;
  const uint32_t lastWordMask = ~uint32_t(0) << lastWordUnusedBits;
  const uint32_t modifier = (contentColor == WHITE) ? ~uint32_t(0) : 0;
  const uint32_t* const data = this->data();

  int bottom = -1;  // inclusive
  const uint32_t* line = data + h * wpl;
  for (int y = h - 1; y >= 0; --y) {
    line -= wpl;
    if (!isLineMonotone(line, lastWordIdx, lastWordMask, modifier)) {
      bottom = y;
      break;
    }
  }

  if (bottom == -1) {
    return QRect();
  }

  int top = bottom;
  line = data;
  for (int y = 0; y < bottom; ++y, line += wpl) {
    if (!isLineMonotone(line, lastWordIdx, lastWordMask, modifier)) {
      top = y;
      break;
    }
  }

  // These are offsets from the corresponding side.
  int left = w;
  int right = w;

  assert(line == data + top * wpl);
  // All lines above top and below bottom are empty.
  for (int y = top; y <= bottom; ++y, line += wpl) {
    if (left != 0) {
      left = leftmostBitOffset(line, left, modifier);
    }
    if (right != 0) {
      const uint32_t word = (line[lastWordIdx] ^ modifier) >> lastWordUnusedBits;
      if (word) {
        const int offset = countLeastSignificantZeroes(word);
        if (offset < right) {
          right = offset;
        }
      } else if (right > lastWordBits) {
        right -= lastWordBits;
        right = rightmostBitOffset(line + lastWordIdx, right, modifier);
        right += lastWordBits;
      }
    }
  }

  // bottom is inclusive, right is a positive offset from width.
  return QRect(left, top, w - right - left, bottom - top + 1);
}  // BinaryImage::contentBoundingBox

void BinaryImage::setPixel(int x, int y, BWColor color) {
  uint32_t* line = this->data() + m_wpl * y;
  (color == WHITE) ? line[x >> 5] &= ~(0x80000000 >> (x & 31)) : line[x >> 5] |= (0x80000000 >> (x & 31));
}

BWColor BinaryImage::getPixel(int x, int y) const {
  const uint32_t* line = this->data() + m_wpl * y;
  return (BWColor)((line[x >> 5] >> (31 - (x & 31))) & 1);
}

uint32_t* BinaryImage::data() {
  if (isNull()) {
    return nullptr;
  }

  copyIfShared();
  return m_data->data();
}

const uint32_t* BinaryImage::data() const {
  if (isNull()) {
    return nullptr;
  }
  return m_data->data();
}

QImage BinaryImage::toQImage() const {
  if (isNull()) {
    return QImage();
  }

  QImage dst(m_width, m_height, QImage::Format_Mono);
  assert(dst.bytesPerLine() % 4 == 0);
  dst.setColorCount(2);
  dst.setColor(0, 0xffffffff);
  dst.setColor(1, 0xff000000);
  const int dstWpl = dst.bytesPerLine() / 4;
  auto* dstLine = (uint32_t*) dst.bits();
  const uint32_t* srcLine = data();
  const int srcWpl = m_wpl;

  for (int i = m_height; i > 0; --i) {
    for (int j = 0; j < srcWpl; ++j) {
      dstLine[j] = qToBigEndian(srcLine[j]);
    }
    srcLine += srcWpl;
    dstLine += dstWpl;
  }
  return dst;
}

QImage BinaryImage::toAlphaMask(const QColor& color) const {
  if (isNull()) {
    return QImage();
  }

  const int alpha = color.alpha();
  const int red = (color.red() * alpha + 128) / 255;
  const int green = (color.green() * alpha + 128) / 255;
  const int blue = (color.blue() * alpha + 128) / 255;
  const uint32_t colors[] = {
      0,                              // replaces white
      qRgba(red, green, blue, alpha)  // replaces black
  };

  const int width = m_width;
  const int height = m_height;

  QImage dst(width, height, QImage::Format_ARGB32_Premultiplied);
  assert(dst.bytesPerLine() % 4 == 0);

  const int dstStride = dst.bytesPerLine() / 4;
  auto* dstLine = (uint32_t*) dst.bits();

  const uint32_t* srcLine = data();
  const int srcStride = m_wpl;

  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; ++x) {
      dstLine[x] = colors[(srcLine[x >> 5] >> (31 - (x & 31))) & 1];
    }
    srcLine += srcStride;
    dstLine += dstStride;
  }
  return dst;
}  // BinaryImage::toAlphaMask

void BinaryImage::copyIfShared() {
  assert(m_data);
  if (!m_data->isShared()) {
    return;
  }

  const size_t numWords = m_height * m_wpl;
  SharedData* newData = SharedData::create(numWords);
  memcpy(newData->data(), m_data->data(), numWords * 4);
  m_data->unref();
  m_data = newData;
}

void BinaryImage::fillRectImpl(uint32_t* const data, const QRect& rect, const BWColor color) {
  const uint32_t pattern = (color == BLACK) ? ~uint32_t(0) : 0;

  if ((rect.x() == 0) && (rect.width() == m_width)) {
    memset(data + rect.y() * m_wpl, pattern, rect.height() * m_wpl * 4);
    return;
  }

  const uint32_t firstWordIdx = rect.left() >> 5;
  // Note: rect.right() == rect.left() + rect.width() - 1
  const uint32_t lastWordIdx = rect.right() >> 5;
  const uint32_t firstWordMask = ~uint32_t(0) >> (rect.left() & 31);
  const uint32_t lastWordMask = ~uint32_t(0) << (31 - (rect.right() & 31));
  uint32_t* line = data + rect.top() * m_wpl;

  if (firstWordIdx == lastWordIdx) {
    line += firstWordIdx;
    const uint32_t mask = firstWordMask & lastWordMask;
    for (int i = rect.height(); i > 0; --i, line += m_wpl) {
      *line = (*line & ~mask) | (pattern & mask);
    }
    return;
  }

  for (int i = rect.height(); i > 0; --i, line += m_wpl) {
    // First word in a line.
    uint32_t* pword = &line[firstWordIdx];
    *pword = (*pword & ~firstWordMask) | (pattern & firstWordMask);

    uint32_t* lastPword = &line[lastWordIdx];
    for (++pword; pword != lastPword; ++pword) {
      *pword = pattern;
    }
    // Last word in a line.
    *pword = (*pword & ~lastWordMask) | (pattern & lastWordMask);
  }
}  // BinaryImage::fillRectImpl

BinaryImage BinaryImage::fromMono(const QImage& image) {
  const int width = image.width();
  const int height = image.height();

  assert(image.bytesPerLine() % 4 == 0);
  const int srcWpl = image.bytesPerLine() / 4;
  const auto* srcLine = (const uint32_t*) image.bits();

  BinaryImage dst(width, height);
  const int dstWpl = dst.wordsPerLine();
  uint32_t* dstLine = dst.data();

  uint32_t modifier = ~uint32_t(0);
  if (image.colorCount() >= 2) {
    if (qGray(image.color(0)) > qGray(image.color(1))) {
      // if color 0 is lighter than color 1
      modifier = ~modifier;
    }
  }

  for (int i = height; i > 0; --i) {
    for (int j = 0; j < dstWpl; ++j) {
      dstLine[j] = qFromBigEndian(srcLine[j]) ^ modifier;
    }
    srcLine += srcWpl;
    dstLine += dstWpl;
  }
  return dst;
}

BinaryImage BinaryImage::fromMono(const QImage& image, const QRect& rect) {
  const int width = rect.width();
  const int height = rect.height();

  assert(image.bytesPerLine() % 4 == 0);
  const int srcWpl = image.bytesPerLine() / 4;
  const auto* srcLine = (const uint32_t*) image.bits();
  srcLine += rect.top() * srcWpl;
  srcLine += rect.left() >> 5;
  const int word1UnusedBits = rect.left() & 31;
  const int word2UnusedBits = 32 - word1UnusedBits;

  BinaryImage dst(width, height);
  const int dstWpl = dst.wordsPerLine();
  uint32_t* dstLine = dst.data();
  const int dstLastWordUnusedBits = (dstWpl << 5) - width;

  uint32_t modifier = ~uint32_t(0);
  if (image.colorCount() >= 2) {
    if (qGray(image.color(0)) > qGray(image.color(1))) {
      // if color 0 is lighter than color 1
      modifier = ~modifier;
    }
  }

  if (word1UnusedBits == 0) {
    // It's not just an optimization.  The code in the other branch
    // is not going to work for this case because uint32_t << 32
    // does not actually clear the word.
    for (int i = height; i > 0; --i) {
      for (int j = 0; j < dstWpl; ++j) {
        dstLine[j] = qFromBigEndian(srcLine[j]) ^ modifier;
      }
      srcLine += srcWpl;
      dstLine += dstWpl;
    }
  } else {
    const int lastWordIdx = (width - 1) >> 5;
    for (int i = height; i > 0; --i) {
      int j = 0;
      uint32_t nextWord = qFromBigEndian(srcLine[j]);
      for (; j < lastWordIdx; ++j) {
        const uint32_t thisWord = nextWord;
        nextWord = qFromBigEndian(srcLine[j + 1]);
        const uint32_t dstWord = (thisWord << word1UnusedBits) | (nextWord >> word2UnusedBits);
        dstLine[j] = dstWord ^ modifier;
      }
      // The last word needs special attention, because srcLine[j + 1]
      // might be outside of the image buffer.
      uint32_t lastWord = nextWord << word1UnusedBits;
      if (dstLastWordUnusedBits < word1UnusedBits) {
        lastWord |= qFromBigEndian(srcLine[j + 1]) >> word2UnusedBits;
      }
      dstLine[j] = lastWord ^ modifier;

      srcLine += srcWpl;
      dstLine += dstWpl;
    }
  }
  return dst;
}  // BinaryImage::fromMono

BinaryImage BinaryImage::fromMonoLSB(const QImage& image) {
  return fromMono(image.convertToFormat(QImage::Format_Mono));
}

BinaryImage BinaryImage::fromMonoLSB(const QImage& image, const QRect& rect) {
  return fromMono(image.convertToFormat(QImage::Format_Mono), rect);
}

BinaryImage BinaryImage::fromIndexed8(const QImage& image, const QRect& rect, const int threshold) {
  const int width = rect.width();
  const int height = rect.height();

  const int srcBpl = image.bytesPerLine();
  const uint8_t* srcLine = image.bits();
  srcLine += rect.top() * srcBpl + rect.left();

  BinaryImage dst(width, height);
  const int dstWpl = dst.wordsPerLine();
  uint32_t* dstLine = dst.data();
  const int lastWordIdx = (width - 1) >> 5;
  const int lastWordBits = width - (lastWordIdx << 5);
  const int lastWordUnusedBits = 32 - lastWordBits;

  const int numColors = image.colorCount();
  assert(numColors <= 256);
  int color_to_gray[256];
  int colorIdx = 0;
  for (; colorIdx < numColors; ++colorIdx) {
    color_to_gray[colorIdx] = qGray(image.color(colorIdx));
  }
  for (; colorIdx < 256; ++colorIdx) {
    color_to_gray[colorIdx] = 0;  // just in case
  }

  for (int i = height; i > 0; --i) {
    for (int j = 0; j < lastWordIdx; ++j) {
      const uint8_t* const srcPos = &srcLine[j << 5];
      uint32_t word = 0;
      for (int bit = 0; bit < 32; ++bit) {
        word <<= 1;
        if (color_to_gray[srcPos[bit]] < threshold) {
          word |= uint32_t(1);
        }
      }
      dstLine[j] = word;
    }

    // Handle the last word.
    const uint8_t* const srcPos = &srcLine[lastWordIdx << 5];
    uint32_t word = 0;
    for (int bit = 0; bit < lastWordBits; ++bit) {
      word <<= 1;
      if (color_to_gray[srcPos[bit]] < threshold) {
        word |= uint32_t(1);
      }
    }
    word <<= lastWordUnusedBits;
    dstLine[lastWordIdx] = word;

    dstLine += dstWpl;
    srcLine += srcBpl;
  }
  return dst;
}  // BinaryImage::fromIndexed8

static inline uint32_t thresholdRgb32(const QRgb c, const int threshold) {
  // gray = (R * 11 + G * 16 + B * 5) / 32;
  // return (gray < threshold) ? 1 : 0;

  const int sum = qRed(c) * 11 + qGreen(c) * 16 + qBlue(c) * 5;
  return (sum < threshold * 32) ? 1 : 0;
}

BinaryImage BinaryImage::fromRgb32(const QImage& image, const QRect& rect, const int threshold) {
  const int width = rect.width();
  const int height = rect.height();

  assert(image.bytesPerLine() % 4 == 0);
  const int srcWpl = image.bytesPerLine() / 4;
  const auto* srcLine = (const QRgb*) image.bits();
  srcLine += rect.top() * srcWpl + rect.left();

  BinaryImage dst(width, height);
  const int dstWpl = dst.wordsPerLine();
  uint32_t* dstLine = dst.data();
  const int lastWordIdx = (width - 1) >> 5;
  const int lastWordBits = width - (lastWordIdx << 5);
  const int lastWordUnusedBits = 32 - lastWordBits;

  for (int i = height; i > 0; --i) {
    for (int j = 0; j < lastWordIdx; ++j) {
      const QRgb* const srcPos = &srcLine[j << 5];
      uint32_t word = 0;
      for (int bit = 0; bit < 32; ++bit) {
        word <<= 1;
        word |= thresholdRgb32(srcPos[bit], threshold);
      }
      dstLine[j] = word;
    }

    // Handle the last word.
    const QRgb* const srcPos = &srcLine[lastWordIdx << 5];
    uint32_t word = 0;
    for (int bit = 0; bit < lastWordBits; ++bit) {
      word <<= 1;
      word |= thresholdRgb32(srcPos[bit], threshold);
    }
    word <<= lastWordUnusedBits;
    dstLine[lastWordIdx] = word;

    dstLine += dstWpl;
    srcLine += srcWpl;
  }
  return dst;
}  // BinaryImage::fromRgb32

static inline uint32_t thresholdArgbPM(const QRgb pm, const int threshold) {
  const int alpha = qAlpha(pm);
  if (alpha == 0) {
    return 1;  // black
  }

  // R = R_PM * 255 / alpha;
  // G = G_PM * 255 / alpha;
  // B = B_PM * 255 / alpha;
  // gray = (R * 11 + G * 16 + B * 5) / 32;
  // return (gray < threshold) ? 1 : 0;

  const int sum = qRed(pm) * (255 * 11) + qGreen(pm) * (255 * 16) + qBlue(pm) * (255 * 5);
  return (sum < alpha * threshold * 32) ? 1 : 0;
}

BinaryImage BinaryImage::fromArgb32Premultiplied(const QImage& image, const QRect& rect, const int threshold) {
  const int width = rect.width();
  const int height = rect.height();

  assert(image.bytesPerLine() % 4 == 0);
  const int srcWpl = image.bytesPerLine() / 4;
  const auto* srcLine = (const QRgb*) image.bits();
  srcLine += rect.top() * srcWpl + rect.left();

  BinaryImage dst(width, height);
  const int dstWpl = dst.wordsPerLine();
  uint32_t* dstLine = dst.data();
  const int lastWordIdx = (width - 1) >> 5;
  const int lastWordBits = width - (lastWordIdx << 5);
  const int lastWordUnusedBits = 32 - lastWordBits;

  for (int i = height; i > 0; --i) {
    for (int j = 0; j < lastWordIdx; ++j) {
      const QRgb* const srcPos = &srcLine[j << 5];
      uint32_t word = 0;
      for (int bit = 0; bit < 32; ++bit) {
        word <<= 1;
        word |= thresholdArgbPM(srcPos[bit], threshold);
      }
      dstLine[j] = word;
    }

    // Handle the last word.
    const QRgb* const srcPos = &srcLine[lastWordIdx << 5];
    uint32_t word = 0;
    for (int bit = 0; bit < lastWordBits; ++bit) {
      word <<= 1;
      word |= thresholdArgbPM(srcPos[bit], threshold);
    }
    word <<= lastWordUnusedBits;
    dstLine[lastWordIdx] = word;

    dstLine += dstWpl;
    srcLine += srcWpl;
  }
  return dst;
}  // BinaryImage::fromArgb32Premultiplied

static inline uint32_t thresholdRgb16(const uint16_t c16, const int threshold) {
  const int c = c16;

  // rgb16: RRRRR GGGGGG BBBBB
  // 43210 543210 43210

  // r8 = r5 * 8 + r5 / 4 = RRRRR RRR
  // 43210 432
  const int r8 = ((c >> 8) & 0xF8) | ((c >> 13) & 0x07);

  // g8 = g6 * 4 + g6 / 16 = GGGGGG GG
  // 543210 54
  const int g8 = ((c >> 3) & 0xFC) | ((c >> 9) & 0x03);

  // b8 = b5 * 8 + b5 / 4 = BBBBB BBB
  // 43210 432
  const int b8 = ((c << 3) & 0xF8) | ((c >> 2) & 0x07);

  // gray = (R * 11 + G * 16 + B * 5) / 32;
  // return (gray < threshold) ? 1 : 0;

  const int sum = r8 * 11 + g8 * 16 + b8 * 5;
  return (sum < threshold * 32) ? 1 : 0;
}

BinaryImage BinaryImage::fromRgb16(const QImage& image, const QRect& rect, const int threshold) {
  const int width = rect.width();
  const int height = rect.height();

  assert(image.bytesPerLine() % 4 == 0);
  const int srcWpl = image.bytesPerLine() / 2;
  const auto* srcLine = (const uint16_t*) image.bits();

  BinaryImage dst(width, height);
  const int dstWpl = dst.wordsPerLine();
  uint32_t* dstLine = dst.data();
  const int lastWordIdx = (width - 1) >> 5;
  const int lastWordBits = width - (lastWordIdx << 5);

  for (int i = height; i > 0; --i) {
    for (int j = 0; j < lastWordIdx; ++j) {
      const uint16_t* const srcPos = &srcLine[j << 5];
      uint32_t word = 0;
      for (int bit = 0; bit < 32; ++bit) {
        word <<= 1;
        word |= thresholdRgb16(srcPos[bit], threshold);
      }
      dstLine[j] = word;
    }

    // Handle the last word.
    const uint16_t* const srcPos = &srcLine[lastWordIdx << 5];
    uint32_t word = 0;
    for (int bit = 0; bit < lastWordBits; ++bit) {
      word <<= 1;
      word |= thresholdRgb16(srcPos[bit], threshold);
    }
    word <<= 32 - lastWordBits;
    dstLine[lastWordIdx] = word;

    dstLine += dstWpl;
    srcLine += srcWpl;
  }
  return dst;
}  // BinaryImage::fromRgb16

/**
 * \brief Determines if the line is either completely black or completely white.
 *
 * \param line The line.
 * \param lastWordIdx Index of the last (possibly incomplete) word.
 * \param lastWordMask The mask to by applied to the last word.
 * \param modifier If 0, this function check if the line is completely black.
 *        If ~uint32_t(0), this function checks if the line is completely white.
 */
bool BinaryImage::isLineMonotone(const uint32_t* const line,
                                 const int lastWordIdx,
                                 const uint32_t lastWordMask,
                                 const uint32_t modifier) {
  for (int i = 0; i < lastWordIdx; ++i) {
    const uint32_t word = line[i] ^ modifier;
    if (word) {
      return false;
    }
  }
  // The last (possibly incomplete) word.
  const int word = (line[lastWordIdx] ^ modifier) & lastWordMask;
  if (word) {
    return false;
  }
  return true;
}

int BinaryImage::leftmostBitOffset(const uint32_t* const line, const int offsetLimit, const uint32_t modifier) {
  const int numWords = (offsetLimit + 31) >> 5;

  int bitOffset = offsetLimit;

  const uint32_t* pword = line;
  for (int i = 0; i < numWords; ++i, ++pword) {
    const uint32_t word = *pword ^ modifier;
    if (word) {
      bitOffset = (i << 5) + countMostSignificantZeroes(word);
      break;
    }
  }
  return std::min(bitOffset, offsetLimit);
}

int BinaryImage::rightmostBitOffset(const uint32_t* const line, const int offsetLimit, const uint32_t modifier) {
  const int numWords = (offsetLimit + 31) >> 5;

  int bitOffset = offsetLimit;

  const uint32_t* pword = line - 1;  // line points to lastWordIdx, which we skip
  for (int i = 0; i < numWords; ++i, --pword) {
    const uint32_t word = *pword ^ modifier;
    if (word) {
      bitOffset = (i << 5) + countLeastSignificantZeroes(word);
      break;
    }
  }
  return std::min(bitOffset, offsetLimit);
}

bool operator==(const BinaryImage& lhs, const BinaryImage& rhs) {
  if (lhs.data() == rhs.data()) {
    // This will also catch the case when both are null.
    return true;
  }

  if ((lhs.width() != rhs.width()) || (lhs.height() != rhs.height())) {
    // This will also catch the case when one is null while the other is not.
    return false;
  }

  const uint32_t* lhsLine = lhs.data();
  const uint32_t* rhsLine = rhs.data();
  const int lhsWpl = lhs.wordsPerLine();
  const int rhsWpl = rhs.wordsPerLine();
  const int lastBitIdx = lhs.width() - 1;
  const int lastWordIdx = lastBitIdx / 32;
  const uint32_t lastWordMask = ~uint32_t(0) << (31 - lastBitIdx % 32);

  for (int i = lhs.height(); i > 0; --i) {
    int j = 0;
    for (; j < lastWordIdx; ++j) {
      if (lhsLine[j] != rhsLine[j]) {
        return false;
      }
    }
    // Handle the last (possibly incomplete) word.
    if ((lhsLine[j] & lastWordMask) != (rhsLine[j] & lastWordMask)) {
      return false;
    }

    lhsLine += lhsWpl;
    rhsLine += rhsWpl;
  }
  return true;
}  // ==

/*====================== BinaryIamge::SharedData ========================*/

void BinaryImage::SharedData::unref() const {
  if (!m_counter.deref()) {
    this->~SharedData();
    free((void*) this);
  }
}

void* BinaryImage::SharedData::operator new(size_t, const NumWords numWords) {
  SharedData* sd = nullptr;
  void* addr = malloc(((char*) &sd->m_data[0] - (char*) sd) + numWords.numWords * 4);
  if (!addr) {
    throw std::bad_alloc();
  }
  return addr;
}

void BinaryImage::SharedData::operator delete(void* addr, NumWords) {
  free(addr);
}
}  // namespace imageproc