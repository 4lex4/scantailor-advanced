// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "ConnectivityMap.h"
#include <QDebug>
#include <QImage>
#include "BinaryImage.h"
#include "BitOps.h"
#include "InfluenceMap.h"

namespace imageproc {
const uint32_t ConnectivityMap::BACKGROUND = ~uint32_t(0);
const uint32_t ConnectivityMap::UNTAGGED_FG = BACKGROUND - 1;

ConnectivityMap::ConnectivityMap() : m_plainData(nullptr), m_size(), m_stride(0), m_maxLabel(0) {}

ConnectivityMap::ConnectivityMap(const QSize& size) : m_plainData(nullptr), m_size(size), m_stride(0), m_maxLabel(0) {
  if (m_size.isEmpty()) {
    return;
  }

  const int width = m_size.width();
  const int height = m_size.height();

  m_data.resize((width + 2) * (height + 2), 0);
  m_stride = width + 2;
  m_plainData = &m_data[0] + 1 + m_stride;
}

ConnectivityMap::ConnectivityMap(const BinaryImage& image, const Connectivity conn)
    : m_plainData(nullptr), m_size(image.size()), m_stride(0), m_maxLabel(0) {
  if (m_size.isEmpty()) {
    return;
  }

  const int width = m_size.width();
  const int height = m_size.height();

  m_data.resize((width + 2) * (height + 2), BACKGROUND);
  m_stride = width + 2;
  m_plainData = &m_data[0] + 1 + m_stride;

  uint32_t* dst = m_plainData;
  const int dstStride = m_stride;

  const uint32_t* src = image.data();
  const int srcStride = image.wordsPerLine();

  const uint32_t msb = uint32_t(1) << 31;
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      if (src[x >> 5] & (msb >> (x & 31))) {
        dst[x] = UNTAGGED_FG;
      }
    }
    src += srcStride;
    dst += dstStride;
  }

  assignIds(conn);
}

ConnectivityMap::ConnectivityMap(const ConnectivityMap& other)
    : m_data(other.m_data),
      m_plainData(nullptr),
      m_size(other.size()),
      m_stride(other.stride()),
      m_maxLabel(other.m_maxLabel) {
  if (!m_size.isEmpty()) {
    m_plainData = &m_data[0] + m_stride + 1;
  }
}

ConnectivityMap::ConnectivityMap(const InfluenceMap& imap)
    : m_plainData(nullptr), m_size(imap.size()), m_stride(imap.stride()), m_maxLabel(imap.maxLabel()) {
  if (m_size.isEmpty()) {
    return;
  }

  m_data.resize((m_size.width() + 2) * (m_size.height() + 2));
  copyFromInfluenceMap(imap);
}

ConnectivityMap& ConnectivityMap::operator=(const ConnectivityMap& other) {
  ConnectivityMap(other).swap(*this);

  return *this;
}

ConnectivityMap& ConnectivityMap::operator=(const InfluenceMap& imap) {
  if ((m_size == imap.size()) && !m_size.isEmpty()) {
    // Common case optimization.
    copyFromInfluenceMap(imap);
  } else {
    ConnectivityMap(imap).swap(*this);
  }

  return *this;
}

void ConnectivityMap::swap(ConnectivityMap& other) {
  m_data.swap(other.m_data);
  std::swap(m_plainData, other.m_plainData);
  std::swap(m_size, other.m_size);
  std::swap(m_stride, other.m_stride);
  std::swap(m_maxLabel, other.m_maxLabel);
}

void ConnectivityMap::addComponent(const BinaryImage& image) {
  if (m_size != image.size()) {
    throw std::invalid_argument("ConnectivityMap::addComponent: sizes do not match");
  }
  if (m_size.isEmpty()) {
    return;
  }

  const int width = m_size.width();
  const int height = m_size.height();

  uint32_t* dst = m_plainData;
  const int dstStride = m_stride;

  const uint32_t* src = image.data();
  const int srcStride = image.wordsPerLine();

  const uint32_t newLabel = m_maxLabel + 1;
  const uint32_t msb = uint32_t(1) << 31;
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      if (src[x >> 5] & (msb >> (x & 31))) {
        dst[x] = newLabel;
      }
    }
    src += srcStride;
    dst += dstStride;
  }

  m_maxLabel = newLabel;
}  // ConnectivityMap::addComponent

void ConnectivityMap::addComponents(const BinaryImage& image, const Connectivity conn) {
  if (m_size != image.size()) {
    throw std::invalid_argument("ConnectivityMap::addComponents: sizes don't match");
  }
  if (m_size.isEmpty()) {
    return;
  }

  addComponents(ConnectivityMap(image, conn));
}

void ConnectivityMap::addComponents(const ConnectivityMap& other) {
  if (m_size != other.m_size) {
    throw std::invalid_argument("ConnectivityMap::addComponents: sizes don't match");
  }
  if (m_size.isEmpty()) {
    return;
  }

  const int width = m_size.width();
  const int height = m_size.height();

  uint32_t* dstLine = m_plainData;
  const int dstStride = m_stride;

  const uint32_t* srcLine = other.m_plainData;
  const int srcStride = other.m_stride;

  uint32_t newMaxLabel = m_maxLabel;
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      const uint32_t srcLabel = srcLine[x];
      if (srcLabel == 0) {
        continue;
      }

      const uint32_t dstLabel = m_maxLabel + srcLabel;
      newMaxLabel = std::max(newMaxLabel, dstLabel);

      dstLine[x] = dstLabel;
    }
    srcLine += srcStride;
    dstLine += dstStride;
  }

  m_maxLabel = newMaxLabel;
}

void ConnectivityMap::removeComponents(const std::unordered_set<uint32_t>& labelsSet) {
  if (m_size.isEmpty() || labelsSet.empty()) {
    return;
  }

  std::vector<uint32_t> map(m_maxLabel, 0);
  uint32_t nextLabel = 1;
  for (uint32_t i = 0; i < m_maxLabel; i++) {
    if (labelsSet.find(i + 1) == labelsSet.end()) {
      map[i] = nextLabel;
      nextLabel++;
    }
  }

  for (uint32_t& label : m_data) {
    if (label != 0) {
      label = map[label - 1];
    }
  }

  m_maxLabel = nextLabel - 1;
}

BinaryImage ConnectivityMap::getBinaryMask() const {
  if (m_size.isEmpty()) {
    return BinaryImage();
  }

  BinaryImage dst(m_size, WHITE);

  const int width = m_size.width();
  const int height = m_size.height();

  uint32_t* dstLine = dst.data();
  const int dstStride = dst.wordsPerLine();

  const uint32_t* srcLine = m_plainData;
  const int srcStride = m_stride;

  const uint32_t msb = uint32_t(1) << 31;
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      if (srcLine[x] != 0) {
        dstLine[x >> 5] |= (msb >> (x & 31));
      }
    }
    srcLine += srcStride;
    dstLine += dstStride;
  }

  return dst;
}

QImage ConnectivityMap::visualized(QColor bgColor) const {
  if (m_size.isEmpty()) {
    return QImage();
  }

  const int width = m_size.width();
  const int height = m_size.height();
  // Convert to premultiplied RGBA.
  bgColor = bgColor.toRgb();
  bgColor.setRedF(bgColor.redF() * bgColor.alphaF());
  bgColor.setGreenF(bgColor.greenF() * bgColor.alphaF());
  bgColor.setBlueF(bgColor.blueF() * bgColor.alphaF());

  QImage dst(m_size, QImage::Format_ARGB32);
  dst.fill(bgColor.rgba());

  const uint32_t* srcLine = m_plainData;
  const int srcStride = m_stride;

  auto* dstLine = reinterpret_cast<uint32_t*>(dst.bits());
  const int dstStride = dst.bytesPerLine() / sizeof(uint32_t);

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      const uint32_t val = srcLine[x];
      if (val == 0) {
        continue;
      }

      const int bitsUnused = countMostSignificantZeroes(val);
      const uint32_t reversed = reverseBits(val) >> bitsUnused;
      const uint32_t mask = ~uint32_t(0) >> bitsUnused;

      const double H = 0.99 * (double(reversed) / mask);
      const double S = 1.0;
      const double V = 1.0;
      QColor color;
      color.setHsvF(H, S, V, 1.0);

      dstLine[x] = color.rgba();
    }
    srcLine += srcStride;
    dstLine += dstStride;
  }

  return dst;
}  // ConnectivityMap::visualized

void ConnectivityMap::copyFromInfluenceMap(const InfluenceMap& imap) {
  assert(!imap.size().isEmpty());
  assert(imap.size() == m_size);

  const int width = m_size.width() + 2;
  const int height = m_size.height() + 2;

  uint32_t* dst = &m_data[0];
  const InfluenceMap::Cell* src = imap.paddedData();
  for (int todo = width * height; todo > 0; --todo) {
    *dst = src->label;
    ++dst;
    ++src;
  }
}

void ConnectivityMap::assignIds(const Connectivity conn) {
  const uint32_t numInitialTags = initialTagging();
  std::vector<uint32_t> table(numInitialTags, 0);

  switch (conn) {
    case CONN4:
      spreadMin4();
      break;
    case CONN8:
      spreadMin8();
      break;
  }

  markUsedIds(table);

  uint32_t nextLabel = 1;
  for (uint32_t i = 0; i < numInitialTags; ++i) {
    if (table[i]) {
      table[i] = nextLabel;
      ++nextLabel;
    }
  }

  remapIds(table);

  m_maxLabel = nextLabel - 1;
}

/**
 * Tags every object pixel that has a non-object pixel to the left.
 */
uint32_t ConnectivityMap::initialTagging() {
  const int width = m_size.width();
  const int height = m_size.height();

  uint32_t nextLabel = 1;

  uint32_t* line = m_plainData;
  const int stride = m_stride;

  for (int y = 0; y < height; ++y, line += stride) {
    for (int x = 0; x < width; ++x) {
      if ((line[x - 1] == BACKGROUND) && (line[x] == UNTAGGED_FG)) {
        line[x] = nextLabel;
        ++nextLabel;
      }
    }
  }

  return nextLabel - 1;
}

void ConnectivityMap::spreadMin4() {
  const int width = m_size.width();
  const int height = m_size.height();
  const int stride = m_stride;

  uint32_t* line = m_plainData;
  uint32_t* prevLine = m_plainData - stride;
  // Top to bottom.
  for (int y = 0; y < height; ++y) {
    // Left to right.
    for (int x = 0; x < width; ++x) {
      if (line[x] == BACKGROUND) {
        continue;
      }
      line[x] = std::min(prevLine[x], std::min(line[x - 1], line[x]));
    }

    prevLine = line;
    line += stride;
  }

  prevLine = line;
  line -= stride;

  FastQueue<uint32_t*> queue;

  // Bottom to top.
  for (int y = height - 1; y >= 0; --y) {
    // Right to left.
    for (int x = width - 1; x >= 0; --x) {
      if (line[x] == BACKGROUND) {
        continue;
      }

      const uint32_t newVal = std::min(line[x + 1], prevLine[x]);

      if (newVal >= line[x]) {
        continue;
      }

      line[x] = newVal;
      // We compare newVal + 1 < neighbor + 1 to
      // make BACKGROUND neighbors overflow and become
      // zero.
      const uint32_t nvp1 = newVal + 1;
      if ((nvp1 < line[x + 1] + 1) || (nvp1 < prevLine[x] + 1)) {
        queue.push(&line[x]);
      }
    }
    prevLine = line;
    line -= stride;
  }

  processQueue4(queue);
}  // ConnectivityMap::spreadMin4

void ConnectivityMap::spreadMin8() {
  const int width = m_size.width();
  const int height = m_size.height();
  const int stride = m_stride;

  uint32_t* line = m_plainData;
  uint32_t* prevLine = m_plainData - stride;
  // Top to bottom.
  for (int y = 0; y < height; ++y) {
    // Left to right.
    for (int x = 0; x < width; ++x) {
      if (line[x] == BACKGROUND) {
        continue;
      }
      line[x]
          = std::min(std::min(std::min(prevLine[x - 1], prevLine[x]), std::min(prevLine[x + 1], line[x - 1])), line[x]);
    }

    prevLine = line;
    line += stride;
  }

  prevLine = line;
  line -= stride;

  FastQueue<uint32_t*> queue;

  // Bottom to top.
  for (int y = height - 1; y >= 0; --y) {
    for (int x = width - 1; x >= 0; --x) {
      if (line[x] == BACKGROUND) {
        continue;
      }

      const uint32_t newVal = std::min(std::min(prevLine[x - 1], prevLine[x]), std::min(prevLine[x + 1], line[x + 1]));

      if (newVal >= line[x]) {
        continue;
      }

      line[x] = newVal;

      // We compare newVal + 1 < neighbor + 1 to
      // make BACKGROUND neighbors overflow and become
      // zero.
      const uint32_t nvp1 = newVal + 1;
      if ((nvp1 < prevLine[x - 1] + 1) || (nvp1 < prevLine[x] + 1) || (nvp1 < prevLine[x + 1] + 1)
          || (nvp1 < line[x + 1] + 1)) {
        queue.push(&line[x]);
      }
    }

    prevLine = line;
    line -= stride;
  }

  processQueue8(queue);
}  // ConnectivityMap::spreadMin8

void ConnectivityMap::processNeighbor(FastQueue<uint32_t*>& queue, const uint32_t thisVal, uint32_t* neighbor) {
  // *neighbor + 1 will overflow if *neighbor == BACKGROUND,
  // which is what we want.
  if (thisVal + 1 < *neighbor + 1) {
    *neighbor = thisVal;
    queue.push(neighbor);
  }
}

void ConnectivityMap::processQueue4(FastQueue<uint32_t*>& queue) {
  const int stride = m_stride;

  while (!queue.empty()) {
    uint32_t* p = queue.front();
    queue.pop();

    const uint32_t thisVal = *p;

    // Northern neighbor.
    p -= stride;
    processNeighbor(queue, thisVal, p);

    // Eastern neighbor.
    p += stride + 1;
    processNeighbor(queue, thisVal, p);

    // Southern neighbor.
    p += stride - 1;
    processNeighbor(queue, thisVal, p);
    // Western neighbor.
    p -= stride + 1;
    processNeighbor(queue, thisVal, p);
  }
}

void ConnectivityMap::processQueue8(FastQueue<uint32_t*>& queue) {
  const int stride = m_stride;

  while (!queue.empty()) {
    uint32_t* p = queue.front();
    queue.pop();

    const uint32_t thisVal = *p;

    // Northern neighbor.
    p -= stride;
    processNeighbor(queue, thisVal, p);

    // North-eastern neighbor.
    ++p;
    processNeighbor(queue, thisVal, p);

    // Eastern neighbor.
    p += stride;
    processNeighbor(queue, thisVal, p);

    // South-eastern neighbor.
    p += stride;
    processNeighbor(queue, thisVal, p);

    // Southern neighbor.
    --p;
    processNeighbor(queue, thisVal, p);

    // South-western neighbor.
    --p;
    processNeighbor(queue, thisVal, p);

    // Western neighbor.
    p -= stride;
    processNeighbor(queue, thisVal, p);
    // North-western neighbor.
    p -= stride;
    processNeighbor(queue, thisVal, p);
  }
}  // ConnectivityMap::processQueue8

void ConnectivityMap::markUsedIds(std::vector<uint32_t>& usedMap) const {
  const int width = m_size.width();
  const int height = m_size.height();
  const int stride = m_stride;

  const uint32_t* line = m_plainData;
  // Top to bottom.
  for (int y = 0; y < height; ++y, line += stride) {
    // Left to right.
    for (int x = 0; x < width; ++x) {
      if (line[x] == BACKGROUND) {
        continue;
      }
      usedMap[line[x] - 1] = 1;
    }
  }
}

void ConnectivityMap::remapIds(const std::vector<uint32_t>& map) {
  for (uint32_t& label : m_data) {
    if (label == BACKGROUND) {
      label = 0;
    } else {
      label = map[label - 1];
    }
  }
}
}  // namespace imageproc
