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

#include "ConnectivityMap.h"
#include <QDebug>
#include <QImage>
#include "BinaryImage.h"
#include "BitOps.h"
#include "InfluenceMap.h"

namespace imageproc {
const uint32_t ConnectivityMap::BACKGROUND = ~uint32_t(0);
const uint32_t ConnectivityMap::UNTAGGED_FG = BACKGROUND - 1;

ConnectivityMap::ConnectivityMap() : m_pData(nullptr), m_size(), m_stride(0), m_maxLabel(0) {}

ConnectivityMap::ConnectivityMap(const QSize& size) : m_pData(nullptr), m_size(size), m_stride(0), m_maxLabel(0) {
  if (m_size.isEmpty()) {
    return;
  }

  const int width = m_size.width();
  const int height = m_size.height();

  m_data.resize((width + 2) * (height + 2), 0);
  m_stride = width + 2;
  m_pData = &m_data[0] + 1 + m_stride;
}

ConnectivityMap::ConnectivityMap(const BinaryImage& image, const Connectivity conn)
    : m_pData(nullptr), m_size(image.size()), m_stride(0), m_maxLabel(0) {
  if (m_size.isEmpty()) {
    return;
  }

  const int width = m_size.width();
  const int height = m_size.height();

  m_data.resize((width + 2) * (height + 2), BACKGROUND);
  m_stride = width + 2;
  m_pData = &m_data[0] + 1 + m_stride;

  uint32_t* dst = m_pData;
  const int dst_stride = m_stride;

  const uint32_t* src = image.data();
  const int src_stride = image.wordsPerLine();

  const uint32_t msb = uint32_t(1) << 31;
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      if (src[x >> 5] & (msb >> (x & 31))) {
        dst[x] = UNTAGGED_FG;
      }
    }
    src += src_stride;
    dst += dst_stride;
  }

  assignIds(conn);
}

ConnectivityMap::ConnectivityMap(const ConnectivityMap& other)
    : m_data(other.m_data),
      m_pData(nullptr),
      m_size(other.size()),
      m_stride(other.stride()),
      m_maxLabel(other.m_maxLabel) {
  if (!m_size.isEmpty()) {
    m_pData = &m_data[0] + m_stride + 1;
  }
}

ConnectivityMap::ConnectivityMap(const InfluenceMap& imap)
    : m_pData(nullptr), m_size(imap.size()), m_stride(imap.stride()), m_maxLabel(imap.maxLabel()) {
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
  std::swap(m_pData, other.m_pData);
  std::swap(m_size, other.m_size);
  std::swap(m_stride, other.m_stride);
  std::swap(m_maxLabel, other.m_maxLabel);
}

void ConnectivityMap::addComponent(const BinaryImage& image) {
  if (m_size != image.size()) {
    throw std::invalid_argument("ConnectivityMap::addComponent: sizes dont match");
  }
  if (m_size.isEmpty()) {
    return;
  }

  const int width = m_size.width();
  const int height = m_size.height();

  uint32_t* dst = m_pData;
  const int dst_stride = m_stride;

  const uint32_t* src = image.data();
  const int src_stride = image.wordsPerLine();

  const uint32_t new_label = m_maxLabel + 1;
  const uint32_t msb = uint32_t(1) << 31;
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      if (src[x >> 5] & (msb >> (x & 31))) {
        dst[x] = new_label;
      }
    }
    src += src_stride;
    dst += dst_stride;
  }

  m_maxLabel = new_label;
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

  uint32_t* dst_line = m_pData;
  const int dst_stride = m_stride;

  const uint32_t* src_line = other.m_pData;
  const int src_stride = other.m_stride;

  uint32_t new_max_label = m_maxLabel;
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      const uint32_t src_label = src_line[x];
      if (src_label == 0) {
        continue;
      }

      const uint32_t dst_label = m_maxLabel + src_label;
      new_max_label = std::max(new_max_label, dst_label);

      dst_line[x] = dst_label;
    }
    src_line += src_stride;
    dst_line += dst_stride;
  }

  m_maxLabel = new_max_label;
}

void ConnectivityMap::removeComponents(const std::unordered_set<uint32_t>& labelsSet) {
  if (m_size.isEmpty() || labelsSet.empty()) {
    return;
  }

  std::vector<uint32_t> map(m_maxLabel, 0);
  uint32_t next_label = 1;
  for (uint32_t i = 0; i < m_maxLabel; i++) {
    if (labelsSet.find(i + 1) == labelsSet.end()) {
      map[i] = next_label;
      next_label++;
    }
  }

  for (uint32_t& label : m_data) {
    if (label != 0) {
      label = map[label - 1];
    }
  }

  m_maxLabel = next_label - 1;
}

BinaryImage ConnectivityMap::getBinaryMask() const {
  if (m_size.isEmpty()) {
    return BinaryImage();
  }

  BinaryImage dst(m_size, WHITE);

  const int width = m_size.width();
  const int height = m_size.height();

  uint32_t* dst_line = dst.data();
  const int dst_stride = dst.wordsPerLine();

  const uint32_t* src_line = m_pData;
  const int src_stride = m_stride;

  const uint32_t msb = uint32_t(1) << 31;
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      if (src_line[x] != 0) {
        dst_line[x >> 5] |= (msb >> (x & 31));
      }
    }
    src_line += src_stride;
    dst_line += dst_stride;
  }

  return dst;
}

QImage ConnectivityMap::visualized(QColor bg_color) const {
  if (m_size.isEmpty()) {
    return QImage();
  }

  const int width = m_size.width();
  const int height = m_size.height();
  // Convert to premultiplied RGBA.
  bg_color = bg_color.toRgb();
  bg_color.setRedF(bg_color.redF() * bg_color.alphaF());
  bg_color.setGreenF(bg_color.greenF() * bg_color.alphaF());
  bg_color.setBlueF(bg_color.blueF() * bg_color.alphaF());

  QImage dst(m_size, QImage::Format_ARGB32);
  dst.fill(bg_color.rgba());

  const uint32_t* src_line = m_pData;
  const int src_stride = m_stride;

  auto* dst_line = reinterpret_cast<uint32_t*>(dst.bits());
  const int dst_stride = dst.bytesPerLine() / sizeof(uint32_t);

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      const uint32_t val = src_line[x];
      if (val == 0) {
        continue;
      }

      const int bits_unused = countMostSignificantZeroes(val);
      const uint32_t reversed = reverseBits(val) >> bits_unused;
      const uint32_t mask = ~uint32_t(0) >> bits_unused;

      const double H = 0.99 * (double(reversed) / mask);
      const double S = 1.0;
      const double V = 1.0;
      QColor color;
      color.setHsvF(H, S, V, 1.0);

      dst_line[x] = color.rgba();
    }
    src_line += src_stride;
    dst_line += dst_stride;
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
  const uint32_t num_initial_tags = initialTagging();
  std::vector<uint32_t> table(num_initial_tags, 0);

  switch (conn) {
    case CONN4:
      spreadMin4();
      break;
    case CONN8:
      spreadMin8();
      break;
  }

  markUsedIds(table);

  uint32_t next_label = 1;
  for (uint32_t i = 0; i < num_initial_tags; ++i) {
    if (table[i]) {
      table[i] = next_label;
      ++next_label;
    }
  }

  remapIds(table);

  m_maxLabel = next_label - 1;
}

/**
 * Tags every object pixel that has a non-object pixel to the left.
 */
uint32_t ConnectivityMap::initialTagging() {
  const int width = m_size.width();
  const int height = m_size.height();

  uint32_t next_label = 1;

  uint32_t* line = m_pData;
  const int stride = m_stride;

  for (int y = 0; y < height; ++y, line += stride) {
    for (int x = 0; x < width; ++x) {
      if ((line[x - 1] == BACKGROUND) && (line[x] == UNTAGGED_FG)) {
        line[x] = next_label;
        ++next_label;
      }
    }
  }

  return next_label - 1;
}

void ConnectivityMap::spreadMin4() {
  const int width = m_size.width();
  const int height = m_size.height();
  const int stride = m_stride;

  uint32_t* line = m_pData;
  uint32_t* prev_line = m_pData - stride;
  // Top to bottom.
  for (int y = 0; y < height; ++y) {
    // Left to right.
    for (int x = 0; x < width; ++x) {
      if (line[x] == BACKGROUND) {
        continue;
      }
      line[x] = std::min(prev_line[x], std::min(line[x - 1], line[x]));
    }

    prev_line = line;
    line += stride;
  }

  prev_line = line;
  line -= stride;

  FastQueue<uint32_t*> queue;

  // Bottom to top.
  for (int y = height - 1; y >= 0; --y) {
    // Right to left.
    for (int x = width - 1; x >= 0; --x) {
      if (line[x] == BACKGROUND) {
        continue;
      }

      const uint32_t new_val = std::min(line[x + 1], prev_line[x]);

      if (new_val >= line[x]) {
        continue;
      }

      line[x] = new_val;
      // We compare new_val + 1 < neighbor + 1 to
      // make BACKGROUND neighbors overflow and become
      // zero.
      const uint32_t nvp1 = new_val + 1;
      if ((nvp1 < line[x + 1] + 1) || (nvp1 < prev_line[x] + 1)) {
        queue.push(&line[x]);
      }
    }
    prev_line = line;
    line -= stride;
  }

  processQueue4(queue);
}  // ConnectivityMap::spreadMin4

void ConnectivityMap::spreadMin8() {
  const int width = m_size.width();
  const int height = m_size.height();
  const int stride = m_stride;

  uint32_t* line = m_pData;
  uint32_t* prev_line = m_pData - stride;
  // Top to bottom.
  for (int y = 0; y < height; ++y) {
    // Left to right.
    for (int x = 0; x < width; ++x) {
      if (line[x] == BACKGROUND) {
        continue;
      }
      line[x] = std::min(std::min(std::min(prev_line[x - 1], prev_line[x]), std::min(prev_line[x + 1], line[x - 1])),
                         line[x]);
    }

    prev_line = line;
    line += stride;
  }

  prev_line = line;
  line -= stride;

  FastQueue<uint32_t*> queue;

  // Bottom to top.
  for (int y = height - 1; y >= 0; --y) {
    for (int x = width - 1; x >= 0; --x) {
      if (line[x] == BACKGROUND) {
        continue;
      }

      const uint32_t new_val
          = std::min(std::min(prev_line[x - 1], prev_line[x]), std::min(prev_line[x + 1], line[x + 1]));

      if (new_val >= line[x]) {
        continue;
      }

      line[x] = new_val;

      // We compare new_val + 1 < neighbor + 1 to
      // make BACKGROUND neighbors overflow and become
      // zero.
      const uint32_t nvp1 = new_val + 1;
      if ((nvp1 < prev_line[x - 1] + 1) || (nvp1 < prev_line[x] + 1) || (nvp1 < prev_line[x + 1] + 1)
          || (nvp1 < line[x + 1] + 1)) {
        queue.push(&line[x]);
      }
    }

    prev_line = line;
    line -= stride;
  }

  processQueue8(queue);
}  // ConnectivityMap::spreadMin8

void ConnectivityMap::processNeighbor(FastQueue<uint32_t*>& queue, const uint32_t this_val, uint32_t* neighbor) {
  // *neighbor + 1 will overflow if *neighbor == BACKGROUND,
  // which is what we want.
  if (this_val + 1 < *neighbor + 1) {
    *neighbor = this_val;
    queue.push(neighbor);
  }
}

void ConnectivityMap::processQueue4(FastQueue<uint32_t*>& queue) {
  const int stride = m_stride;

  while (!queue.empty()) {
    uint32_t* p = queue.front();
    queue.pop();

    const uint32_t this_val = *p;

    // Northern neighbor.
    p -= stride;
    processNeighbor(queue, this_val, p);

    // Eastern neighbor.
    p += stride + 1;
    processNeighbor(queue, this_val, p);

    // Southern neighbor.
    p += stride - 1;
    processNeighbor(queue, this_val, p);
    // Western neighbor.
    p -= stride + 1;
    processNeighbor(queue, this_val, p);
  }
}

void ConnectivityMap::processQueue8(FastQueue<uint32_t*>& queue) {
  const int stride = m_stride;

  while (!queue.empty()) {
    uint32_t* p = queue.front();
    queue.pop();

    const uint32_t this_val = *p;

    // Northern neighbor.
    p -= stride;
    processNeighbor(queue, this_val, p);

    // North-eastern neighbor.
    ++p;
    processNeighbor(queue, this_val, p);

    // Eastern neighbor.
    p += stride;
    processNeighbor(queue, this_val, p);

    // South-eastern neighbor.
    p += stride;
    processNeighbor(queue, this_val, p);

    // Southern neighbor.
    --p;
    processNeighbor(queue, this_val, p);

    // South-western neighbor.
    --p;
    processNeighbor(queue, this_val, p);

    // Western neighbor.
    p -= stride;
    processNeighbor(queue, this_val, p);
    // North-western neighbor.
    p -= stride;
    processNeighbor(queue, this_val, p);
  }
}  // ConnectivityMap::processQueue8

void ConnectivityMap::markUsedIds(std::vector<uint32_t>& used_map) const {
  const int width = m_size.width();
  const int height = m_size.height();
  const int stride = m_stride;

  const uint32_t* line = m_pData;
  // Top to bottom.
  for (int y = 0; y < height; ++y, line += stride) {
    // Left to right.
    for (int x = 0; x < width; ++x) {
      if (line[x] == BACKGROUND) {
        continue;
      }
      used_map[line[x] - 1] = 1;
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