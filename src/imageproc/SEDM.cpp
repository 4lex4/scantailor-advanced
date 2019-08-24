// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Copyright (C) 2002,2003  Ricardo Fabbri <rfabbri@if.sc.usp.br>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "SEDM.h"
#include "BinaryImage.h"
#include "ConnectivityMap.h"
#include "Morphology.h"
#include "RasterOp.h"
#include "SeedFill.h"

namespace imageproc {
// Note that -1 is an implementation detail.
// It exists to make sure INF_DIST + 1 doesn't overflow.
const uint32_t SEDM::INF_DIST = ~uint32_t(0) - 1;

SEDM::SEDM() : m_plainData(nullptr), m_size(), m_stride(0) {}

SEDM::SEDM(const BinaryImage& image, const DistType distType, const Borders borders)
    : m_plainData(nullptr), m_size(image.size()), m_stride(0) {
  if (image.isNull()) {
    return;
  }

  const int width = m_size.width();
  const int height = m_size.height();

  m_data.resize((width + 2) * (height + 2), INF_DIST);
  m_stride = width + 2;
  m_plainData = &m_data[0] + m_stride + 1;

  if (borders & DIST_TO_TOP_BORDER) {
    memset(&m_data[0], 0, m_stride * sizeof(m_data[0]));
  }
  if (borders & DIST_TO_BOTTOM_BORDER) {
    memset(&m_data[m_data.size() - m_stride], 0, m_stride * sizeof(m_data[0]));
  }
  if (borders & (DIST_TO_LEFT_BORDER | DIST_TO_RIGHT_BORDER)) {
    const int last = m_stride - 1;
    uint32_t* line = &m_data[0];
    for (int todo = height + 2; todo > 0; --todo) {
      if (borders & DIST_TO_LEFT_BORDER) {
        line[0] = 0;
      }
      if (borders & DIST_TO_RIGHT_BORDER) {
        line[last] = 0;
      }
      line += m_stride;
    }
  }

  uint32_t initial_distance[2];
  if (distType == DIST_TO_WHITE) {
    initial_distance[0] = 0;         // white
    initial_distance[1] = INF_DIST;  // black
  } else {
    initial_distance[0] = INF_DIST;  // white
    initial_distance[1] = 0;         // black
  }

  uint32_t* pDist = m_plainData;
  const uint32_t* imgLine = image.data();
  const int imgStride = image.wordsPerLine();
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x, ++pDist) {
      uint32_t word = imgLine[x >> 5];
      word >>= 31 - (x & 31);
      *pDist = initial_distance[word & 1];
    }
    pDist += 2;
    imgLine += imgStride;
  }

  processColumns();
  processRows();
}

SEDM::SEDM(ConnectivityMap& cmap) : m_plainData(nullptr), m_size(cmap.size()), m_stride(0) {
  if (m_size.isEmpty()) {
    return;
  }

  const int width = m_size.width();
  const int height = m_size.height();

  m_data.resize((width + 2) * (height + 2), INF_DIST);
  m_stride = width + 2;
  m_plainData = &m_data[0] + m_stride + 1;

  uint32_t* pDist = m_plainData;
  const uint32_t* pLabel = cmap.data();
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x, ++pDist, ++pLabel) {
      if (*pLabel) {
        *pDist = 0;
      }
    }
    pDist += 2;
    pLabel += 2;
  }

  processColumns(cmap);
  processRows(cmap);
}

SEDM::SEDM(const SEDM& other)
    : m_data(other.m_data), m_plainData(nullptr), m_size(other.m_size), m_stride(other.m_stride) {
  if (!m_size.isEmpty()) {
    m_plainData = &m_data[0] + m_stride + 1;
  }
}

SEDM& SEDM::operator=(const SEDM& other) {
  SEDM(other).swap(*this);

  return *this;
}

void SEDM::swap(SEDM& other) {
  m_data.swap(other.m_data);
  std::swap(m_plainData, other.m_plainData);
  std::swap(m_size, other.m_size);
  std::swap(m_stride, other.m_stride);
}

BinaryImage SEDM::findPeaksDestructive() {
  if (m_size.isEmpty()) {
    return BinaryImage();
  }

  BinaryImage peakCandidates(findPeakCandidatesNonPadded());
  // To check if a peak candidate is really a peak, we have to check
  // that every cell in its neighborhood has a lower value than that
  // candidate.  We are working with 3x3 neighborhoods.
  BinaryImage neighborhoodMask(dilateBrick(peakCandidates, QSize(3, 3), peakCandidates.rect().adjusted(-1, -1, 1, 1)));
  rasterOp<RopXor<RopSrc, RopDst>>(neighborhoodMask, neighborhoodMask.rect().adjusted(1, 1, -1, -1), peakCandidates,
                                   QPoint(0, 0));

  // Cells in the neighborhood of a peak candidate fall into two categories:
  // 1. The cell has a lower value than the peak candidate.
  // 2. The cell has the same value as the peak candidate,
  // but it has a cell with a greater value in its neighborhood.
  // The second case indicates that our candidate is not relly a peak.
  // To test for the second case we are going to increment the values
  // of the cells in the neighborhood of peak candidates, find the peak
  // candidates again and analize the differences.

  incrementMaskedPadded(neighborhoodMask);
  neighborhoodMask.release();

  BinaryImage diff(findPeakCandidatesNonPadded());
  rasterOp<RopXor<RopSrc, RopDst>>(diff, peakCandidates);

  // If a bin that has changed its state was a part of a peak candidate,
  // it means a neighboring bin went from equal to a greater value,
  // which indicates that such candidate is not a peak.
  const BinaryImage notPeaks(seedFill(diff, peakCandidates, CONN8));
  diff.release();

  rasterOp<RopXor<RopSrc, RopDst>>(peakCandidates, notPeaks);

  return peakCandidates;
}  // SEDM::findPeaksDestructive

inline uint32_t SEDM::distSq(const int x1, const int x2, const uint32_t dySq) {
  if (dySq == INF_DIST) {
    return INF_DIST;
  }
  const int dx = x1 - x2;
  const uint32_t dxSq = dx * dx;

  return dxSq + dySq;
}

void SEDM::processColumns() {
  const int width = m_size.width() + 2;
  const int height = m_size.height() + 2;

  uint32_t* pSqd = &m_data[0];
  for (int x = 0; x < width; ++x, ++pSqd) {
    // (d + 1)^2 = d^2 + 2d + 1
    uint32_t b = 1;  // 2d + 1 in the above formula.
    for (int todo = height - 1; todo > 0; --todo) {
      const uint32_t sqd = *pSqd + b;
      pSqd += width;
      if (*pSqd > sqd) {
        *pSqd = sqd;
        b += 2;
      } else {
        b = 1;
      }
    }

    b = 1;
    for (int todo = height - 1; todo > 0; --todo) {
      const uint32_t sqd = *pSqd + b;
      pSqd -= width;
      if (*pSqd > sqd) {
        *pSqd = sqd;
        b += 2;
      } else {
        b = 1;
      }
    }
  }
}  // SEDM::processColumns

void SEDM::processColumns(ConnectivityMap& cmap) {
  const int width = m_size.width() + 2;
  const int height = m_size.height() + 2;

  uint32_t* pSqd = &m_data[0];
  uint32_t* pLabel = cmap.paddedData();
  for (int x = 0; x < width; ++x, ++pSqd, ++pLabel) {
    // (d + 1)^2 = d^2 + 2d + 1
    uint32_t b = 1;  // 2d + 1 in the above formula.
    for (int todo = height - 1; todo > 0; --todo) {
      const uint32_t sqd = *pSqd + b;
      pSqd += width;
      pLabel += width;
      if (sqd < *pSqd) {
        *pSqd = sqd;
        *pLabel = pLabel[-width];
        b += 2;
      } else {
        b = 1;
      }
    }

    b = 1;
    for (int todo = height - 1; todo > 0; --todo) {
      const uint32_t sqd = *pSqd + b;
      pSqd -= width;
      pLabel -= width;
      if (sqd < *pSqd) {
        *pSqd = sqd;
        *pLabel = pLabel[width];
        b += 2;
      } else {
        b = 1;
      }
    }
  }
}  // SEDM::processColumns

void SEDM::processRows() {
  const int width = m_size.width() + 2;
  const int height = m_size.height() + 2;

  std::vector<int> s(width, 0);
  std::vector<int> t(width, 0);
  std::vector<uint32_t> rowCopy(width, 0);

  uint32_t* line = &m_data[0];
  for (int y = 0; y < height; ++y, line += width) {
    int q = 0;
    s[0] = 0;
    t[0] = 0;
    for (int x = 1; x < width; ++x) {
      while (q >= 0 && distSq(t[q], s[q], line[s[q]]) > distSq(t[q], x, line[x])) {
        --q;
      }

      if (q < 0) {
        q = 0;
        s[0] = x;
      } else {
        const int x2 = s[q];
        if ((line[x] != INF_DIST) && (line[x2] != INF_DIST)) {
          int w = (x * x + line[x]) - (x2 * x2 + line[x2]);
          w /= (x - x2) << 1;
          ++w;
          if ((unsigned) w < (unsigned) width) {
            ++q;
            s[q] = x;
            t[q] = w;
          }
        }
      }
    }

    memcpy(&rowCopy[0], line, width * sizeof(*line));

    for (int x = width - 1; x >= 0; --x) {
      const int x2 = s[q];
      line[x] = distSq(x, x2, rowCopy[x2]);
      if (x == t[q]) {
        --q;
      }
    }
  }
}  // SEDM::processRows

void SEDM::processRows(ConnectivityMap& cmap) {
  const int width = m_size.width() + 2;
  const int height = m_size.height() + 2;

  std::vector<int> s(width, 0);
  std::vector<int> t(width, 0);
  std::vector<uint32_t> rowCopy(width, 0);
  std::vector<uint32_t> cmapRowCopy(width, 0);

  uint32_t* line = &m_data[0];
  uint32_t* cmapLine = cmap.paddedData();
  for (int y = 0; y < height; ++y, line += width, cmapLine += width) {
    int q = 0;
    s[0] = 0;
    t[0] = 0;
    for (int x = 1; x < width; ++x) {
      while (q >= 0 && distSq(t[q], s[q], line[s[q]]) > distSq(t[q], x, line[x])) {
        --q;
      }

      if (q < 0) {
        q = 0;
        s[0] = x;
      } else {
        const int x2 = s[q];
        if ((line[x] != INF_DIST) && (line[x2] != INF_DIST)) {
          int w = (x * x + line[x]) - (x2 * x2 + line[x2]);
          w /= (x - x2) << 1;
          ++w;
          if ((unsigned) w < (unsigned) width) {
            ++q;
            s[q] = x;
            t[q] = w;
          }
        }
      }
    }

    memcpy(&rowCopy[0], line, width * sizeof(*line));
    memcpy(&cmapRowCopy[0], cmapLine, width * sizeof(*cmapLine));

    for (int x = width - 1; x >= 0; --x) {
      const int x2 = s[q];
      line[x] = distSq(x, x2, rowCopy[x2]);
      cmapLine[x] = cmapRowCopy[x2];
      if (x == t[q]) {
        --q;
      }
    }
  }
}  // SEDM::processRows

/*====================== Peak finding stuff goes below ====================*/

BinaryImage SEDM::findPeakCandidatesNonPadded() const {
  std::vector<uint32_t> maxed(m_data.size(), 0);

  // Every cell becomes the maximum of itself and its neighbors.
  max3x3(&m_data[0], &maxed[0]);

  return buildEqualMapNonPadded(&m_data[0], &maxed[0]);
}

BinaryImage SEDM::buildEqualMapNonPadded(const uint32_t* src1, const uint32_t* src2) const {
  const int width = m_size.width();
  const int height = m_size.height();

  BinaryImage dst(width, height, WHITE);
  uint32_t* dstLine = dst.data();
  const int dstWpl = dst.wordsPerLine();
  const int srcStride = m_stride;
  const uint32_t* src1Line = src1 + srcStride + 1;
  const uint32_t* src2Line = src2 + srcStride + 1;
  const uint32_t msb = uint32_t(1) << 31;

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      if (std::max(src1Line[x], src2Line[x]) - std::min(src1Line[x], src2Line[x]) == 0) {
        dstLine[x >> 5] |= msb >> (x & 31);
      }
    }
    dstLine += dstWpl;
    src1Line += srcStride;
    src2Line += srcStride;
  }

  return dst;
}

void SEDM::max3x3(const uint32_t* src, uint32_t* dst) const {
  std::vector<uint32_t> tmp(m_data.size(), 0);
  max3x1(src, &tmp[0]);
  max1x3(&tmp[0], dst);
}

void SEDM::max3x1(const uint32_t* src, uint32_t* dst) const {
  const int width = m_size.width() + 2;
  const int height = m_size.height() + 2;

  const uint32_t* srcLine = &src[0];
  uint32_t* dstLine = &dst[0];

  for (int y = 0; y < height; ++y) {
    // First column (no left neighbors).
    int x = 0;
    dstLine[x] = std::max(srcLine[x], srcLine[x + 1]);

    for (++x; x < width - 1; ++x) {
      const uint32_t prev = srcLine[x - 1];
      const uint32_t cur = srcLine[x];
      const uint32_t next = srcLine[x + 1];
      dstLine[x] = std::max(prev, std::max(cur, next));
    }

    // Last column (no right neighbors).
    dstLine[x] = std::max(srcLine[x], srcLine[x - 1]);

    srcLine += width;
    dstLine += width;
  }
}

void SEDM::max1x3(const uint32_t* src, uint32_t* dst) const {
  const int width = m_size.width() + 2;
  const int height = m_size.height() + 2;
  // First row (no top neighbors).
  const uint32_t* pSrc = &src[0];
  uint32_t* pDst = &dst[0];
  for (int x = 0; x < width; ++x) {
    *pDst = std::max(pSrc[0], pSrc[width]);
    ++pSrc;
    ++pDst;
  }

  for (int y = 1; y < height - 1; ++y) {
    for (int x = 0; x < width; ++x) {
      const uint32_t prev = pSrc[x - width];
      const uint32_t cur = pSrc[x];
      const uint32_t next = pSrc[x + width];
      pDst[x] = std::max(prev, std::max(cur, next));
    }

    pSrc += width;
    pDst += width;
  }

  // Last row (no bottom neighbors).
  for (int x = 0; x < width; ++x) {
    *pDst = std::max(pSrc[0], pSrc[-width]);
    ++pSrc;
    ++pDst;
  }
}

void SEDM::incrementMaskedPadded(const BinaryImage& mask) {
  const int width = m_size.width() + 2;
  const int height = m_size.height() + 2;

  uint32_t* dataLine = &m_data[0];
  const uint32_t* maskLine = mask.data();
  const int maskWpl = mask.wordsPerLine();

  const uint32_t msb = uint32_t(1) << 31;
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      if (maskLine[x >> 5] & (msb >> (x & 31))) {
        ++dataLine[x];
      }
    }
    dataLine += width;
    maskLine += maskWpl;
  }
}
}  // namespace imageproc