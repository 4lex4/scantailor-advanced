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

#include "InfluenceMap.h"
#include <QImage>
#include "BinaryImage.h"
#include "BitOps.h"
#include "ConnectivityMap.h"

class QImage;

namespace imageproc {
InfluenceMap::InfluenceMap() : m_pData(nullptr), m_size(), m_stride(0), m_maxLabel(0) {}

InfluenceMap::InfluenceMap(const ConnectivityMap& cmap) : m_pData(nullptr), m_size(), m_stride(0), m_maxLabel(0) {
  if (cmap.size().isEmpty()) {
    return;
  }

  init(cmap);
}

InfluenceMap::InfluenceMap(const ConnectivityMap& cmap, const BinaryImage& mask)
    : m_pData(nullptr), m_size(), m_stride(0), m_maxLabel(0) {
  if (cmap.size().isEmpty()) {
    return;
  }
  if (cmap.size() != mask.size()) {
    throw std::invalid_argument("InfluenceMap: cmap and mask have different sizes");
  }

  init(cmap, &mask);
}

InfluenceMap::InfluenceMap(const InfluenceMap& other)
    : m_data(other.m_data),
      m_pData(nullptr),
      m_size(other.size()),
      m_stride(other.stride()),
      m_maxLabel(other.m_maxLabel) {
  if (!m_size.isEmpty()) {
    m_pData = &m_data[0] + m_stride + 1;
  }
}

InfluenceMap& InfluenceMap::operator=(const InfluenceMap& other) {
  InfluenceMap(other).swap(*this);

  return *this;
}

void InfluenceMap::swap(InfluenceMap& other) {
  m_data.swap(other.m_data);
  std::swap(m_pData, other.m_pData);
  std::swap(m_size, other.m_size);
  std::swap(m_stride, other.m_stride);
  std::swap(m_maxLabel, other.m_maxLabel);
}

void InfluenceMap::init(const ConnectivityMap& cmap, const BinaryImage* mask) {
  const int width = cmap.size().width() + 2;
  const int height = cmap.size().height() + 2;

  m_size = cmap.size();
  m_stride = width;
  m_data.resize(width * height);
  m_pData = &m_data[0] + width + 1;
  m_maxLabel = cmap.maxLabel();

  FastQueue<Cell*> queue;

  Cell* cell = &m_data[0];
  const uint32_t* label = cmap.paddedData();
  for (int i = width * height; i > 0; --i) {
    assert(*label <= cmap.maxLabel());
    cell->label = *label;
    cell->distSq = 0;
    cell->vec.x = 0;
    cell->vec.y = 0;
    if (*label != 0) {
      queue.push(cell);
    }
    ++cell;
    ++label;
  }

  if (mask) {
    const uint32_t* mask_line = mask->data();
    const int mask_stride = mask->wordsPerLine();
    cell = m_pData;
    const uint32_t msb = uint32_t(1) << 31;
    for (int y = 0; y < height - 2; ++y) {
      for (int x = 0; x < width - 2; ++x, ++cell) {
        if (mask_line[x >> 5] & (msb >> (x & 31))) {
          if (cell->label == 0) {
            cell->distSq = ~uint32_t(0);
          }
        }
      }
      mask_line += mask_stride;
      cell += 2;
    }
  } else {
    cell = m_pData;
    for (int y = 0; y < height - 2; ++y) {
      for (int x = 0; x < width - 2; ++x, ++cell) {
        if (cell->label == 0) {
          cell->distSq = ~uint32_t(0);
        }
      }
      cell += 2;
    }
  }

  while (!queue.empty()) {
    cell = queue.front();
    queue.pop();

    assert((cell - &m_data[0]) / width > 0);
    assert((cell - &m_data[0]) / width < height - 1);
    assert((cell - &m_data[0]) % width > 0);
    assert((cell - &m_data[0]) % width < width - 1);
    assert(cell->distSq != ~uint32_t(0));
    assert(cell->label != 0);
    assert(cell->label <= m_maxLabel);

    const int32_t dx2 = cell->vec.x << 1;
    const int32_t dy2 = cell->vec.y << 1;

    // North-western neighbor.
    Cell* nbh = cell - width - 1;
    uint32_t new_dist_sq = cell->distSq + dx2 + dy2 + 2;
    if (new_dist_sq < nbh->distSq) {
      nbh->label = cell->label;
      nbh->distSq = new_dist_sq;
      nbh->vec.x = static_cast<int16_t>(cell->vec.x + 1);
      nbh->vec.y = static_cast<int16_t>(cell->vec.y + 1);
      queue.push(nbh);
    }
    // Northern neighbor.
    ++nbh;
    new_dist_sq = cell->distSq + dy2 + 1;
    if (new_dist_sq < nbh->distSq) {
      nbh->label = cell->label;
      nbh->distSq = new_dist_sq;
      nbh->vec.x = cell->vec.x;
      nbh->vec.y = static_cast<int16_t>(cell->vec.y + 1);
      queue.push(nbh);
    }

    // North-eastern neighbor.
    ++nbh;
    new_dist_sq = cell->distSq - dx2 + dy2 + 2;
    if (new_dist_sq < nbh->distSq) {
      nbh->label = cell->label;
      nbh->distSq = new_dist_sq;
      nbh->vec.x = static_cast<int16_t>(cell->vec.x - 1);
      nbh->vec.y = static_cast<int16_t>(cell->vec.y + 1);
      queue.push(nbh);
    }
    // Eastern neighbor.
    nbh += width;
    new_dist_sq = cell->distSq - dx2 + 1;
    if (new_dist_sq < nbh->distSq) {
      nbh->label = cell->label;
      nbh->distSq = new_dist_sq;
      nbh->vec.x = static_cast<int16_t>(cell->vec.x - 1);
      nbh->vec.y = cell->vec.y;
      queue.push(nbh);
    }

    // South-eastern neighbor.
    nbh += width;
    new_dist_sq = cell->distSq - dx2 - dy2 + 2;
    if (new_dist_sq < nbh->distSq) {
      nbh->label = cell->label;
      nbh->distSq = new_dist_sq;
      nbh->vec.x = static_cast<int16_t>(cell->vec.x - 1);
      nbh->vec.y = static_cast<int16_t>(cell->vec.y - 1);
      queue.push(nbh);
    }
    // Southern neighbor.
    --nbh;
    new_dist_sq = cell->distSq - dy2 + 1;
    if (new_dist_sq < nbh->distSq) {
      nbh->label = cell->label;
      nbh->distSq = new_dist_sq;
      nbh->vec.x = cell->vec.x;
      nbh->vec.y = static_cast<int16_t>(cell->vec.y - 1);
      queue.push(nbh);
    }

    // South-western neighbor.
    --nbh;
    new_dist_sq = cell->distSq + dx2 - dy2 + 2;
    if (new_dist_sq < nbh->distSq) {
      nbh->label = cell->label;
      nbh->distSq = new_dist_sq;
      nbh->vec.x = static_cast<int16_t>(cell->vec.x + 1);
      nbh->vec.y = static_cast<int16_t>(cell->vec.y - 1);
      queue.push(nbh);
    }
    // Western neighbor.
    nbh -= width;
    new_dist_sq = cell->distSq + dx2 + 1;
    if (new_dist_sq < nbh->distSq) {
      nbh->label = cell->label;
      nbh->distSq = new_dist_sq;
      nbh->vec.x = static_cast<int16_t>(cell->vec.x + 1);
      nbh->vec.y = cell->vec.y;
      queue.push(nbh);
    }
  }
}  // InfluenceMap::init

QImage InfluenceMap::visualized() const {
  if (m_size.isEmpty()) {
    return QImage();
  }

  const int width = m_size.width();
  const int height = m_size.height();

  QImage dst(m_size, QImage::Format_ARGB32);
  dst.fill(0x00FFFFFF);  // transparent white
  const Cell* src_line = m_pData;
  const int src_stride = m_stride;

  auto* dst_line = reinterpret_cast<uint32_t*>(dst.bits());
  const int dst_stride = dst.bytesPerLine() / sizeof(uint32_t);

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      const uint32_t val = src_line[x].label;
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
}  // InfluenceMap::visualized
}  // namespace imageproc