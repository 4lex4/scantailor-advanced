// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "HoughLineDetector.h"
#include <QDebug>
#include <QPainter>
#include <cassert>
#include <cmath>
#include "BinaryImage.h"
#include "ConnCompEraser.h"
#include "Constants.h"
#include "Grayscale.h"
#include "Morphology.h"
#include "RasterOp.h"
#include "SeedFill.h"

namespace imageproc {
class HoughLineDetector::GreaterQualityFirst {
 public:
  bool operator()(const HoughLine& lhs, const HoughLine& rhs) const { return lhs.quality() > rhs.quality(); }
};


HoughLineDetector::HoughLineDetector(const QSize& inputDimensions,
                                     const double distanceResolution,
                                     const double startAngle,
                                     const double angleDelta,
                                     const int numAngles)
    : m_distanceResolution(distanceResolution), m_recipDistanceResolution(1.0 / distanceResolution) {
  const int maxX = inputDimensions.width() - 1;
  const int maxY = inputDimensions.height() - 1;

  QPoint checkpoints[3];
  checkpoints[0] = QPoint(maxX, maxY);
  checkpoints[1] = QPoint(maxX, 0);
  checkpoints[2] = QPoint(0, maxY);

  double maxDistance = 0.0;
  double minDistance = 0.0;

  m_angleUnitVectors.reserve(numAngles);
  for (int i = 0; i < numAngles; ++i) {
    double angle = startAngle + angleDelta * i;
    angle *= constants::DEG2RAD;

    const QPointF uv(std::cos(angle), std::sin(angle));
    for (const QPoint& p : checkpoints) {
      const double distance = uv.x() * p.x() + uv.y() * p.y();
      maxDistance = std::max(maxDistance, distance);
      minDistance = std::min(minDistance, distance);
    }

    m_angleUnitVectors.push_back(uv);
  }
  // We bias distances to make them non-negative.
  m_distanceBias = -minDistance;

  const double maxBiasedDistance = maxDistance + m_distanceBias;
  const auto maxBin = int(maxBiasedDistance * m_recipDistanceResolution + 0.5);

  m_histWidth = maxBin + 1;
  m_histHeight = numAngles;
  m_histogram.resize(m_histWidth * m_histHeight, 0);
}

void HoughLineDetector::process(int x, int y, unsigned weight) {
  unsigned* histLine = &m_histogram[0];

  for (const QPointF& uv : m_angleUnitVectors) {
    const double distance = uv.x() * x + uv.y() * y;
    const double biasedDistance = distance + m_distanceBias;

    const auto bin = (int) (biasedDistance * m_recipDistanceResolution + 0.5);
    assert(bin >= 0 && bin < m_histWidth);
    histLine[bin] += weight;

    histLine += m_histWidth;
  }
}

QImage HoughLineDetector::visualizeHoughSpace(const unsigned lowerBound) const {
  QImage intensity(m_histWidth, m_histHeight, QImage::Format_Indexed8);
  intensity.setColorTable(createGrayscalePalette());
  if ((m_histWidth > 0) && (m_histHeight > 0) && intensity.isNull()) {
    throw std::bad_alloc();
  }

  unsigned maxValue = 0;
  const unsigned* histLine = &m_histogram[0];
  for (int y = 0; y < m_histHeight; ++y) {
    for (int x = 0; x < m_histWidth; ++x) {
      maxValue = std::max(maxValue, histLine[x]);
    }
    histLine += m_histWidth;
  }

  if (maxValue == 0) {
    intensity.fill(0);

    return intensity;
  }

  unsigned char* intensityLine = intensity.bits();
  const int intensityBpl = intensity.bytesPerLine();
  histLine = &m_histogram[0];
  for (int y = 0; y < m_histHeight; ++y) {
    for (int x = 0; x < m_histWidth; ++x) {
      const auto intensity = (unsigned) std::floor(histLine[x] * 255.0 / maxValue + 0.5);
      intensityLine[x] = (unsigned char) intensity;
    }
    intensityLine += intensityBpl;
    histLine += m_histWidth;
  }

  const BinaryImage peaks(findHistogramPeaks(m_histogram, m_histWidth, m_histHeight, lowerBound));

  QImage peaksVisual(intensity.size(), QImage::Format_ARGB32_Premultiplied);
  peaksVisual.fill(qRgb(0xff, 0x00, 0x00));
  QImage alphaChannel(peaks.toQImage());
  if (qGray(alphaChannel.color(0)) < qGray(alphaChannel.color(1))) {
    alphaChannel.setColor(0, qRgb(0x80, 0x80, 0x80));
    alphaChannel.setColor(1, 0);
  } else {
    alphaChannel.setColor(0, 0);
    alphaChannel.setColor(1, qRgb(0x80, 0x80, 0x80));
  }
  peaksVisual.setAlphaChannel(alphaChannel);

  QImage visual(intensity.convertToFormat(QImage::Format_ARGB32_Premultiplied));

  {
    QPainter painter(&visual);
    painter.drawImage(QPoint(0, 0), peaksVisual);
  }

  return visual;
}  // HoughLineDetector::visualizeHoughSpace

std::vector<HoughLine> HoughLineDetector::findLines(const unsigned qualityLowerBound) const {
  BinaryImage peaks(findHistogramPeaks(m_histogram, m_histWidth, m_histHeight, qualityLowerBound));

  std::vector<HoughLine> lines;

  const QRect peaksRect(peaks.rect());
  ConnCompEraser eraser(peaks.release(), CONN8);
  ConnComp cc;
  while (!(cc = eraser.nextConnComp()).isNull()) {
    const unsigned level = m_histogram[cc.seed().y() * m_histWidth + cc.seed().x()];

    const QPoint center(cc.rect().center());
    const QPointF normUv(m_angleUnitVectors[center.y()]);
    const double distance = (center.x() + 0.5) * m_distanceResolution - m_distanceBias;
    lines.emplace_back(normUv, distance, level);
  }

  std::sort(lines.begin(), lines.end(), GreaterQualityFirst());

  return lines;
}

BinaryImage HoughLineDetector::findHistogramPeaks(const std::vector<unsigned>& hist,
                                                  const int width,
                                                  const int height,
                                                  const unsigned lowerBound) {
  // Peak candidates are connected components of bins having the same
  // value.  Such a connected component may or may not be a peak.
  BinaryImage peakCandidates(findPeakCandidates(hist, width, height, lowerBound));

  // To check if a peak candidate is really a peak, we have to check
  // that every bin in its neighborhood has a lower value than that
  // candidate.  The are working with 5x5 neighborhoods.
  BinaryImage neighborhoodMask(dilateBrick(peakCandidates, QSize(5, 5)));
  rasterOp<RopXor<RopSrc, RopDst>>(neighborhoodMask, peakCandidates);

  // Bins in the neighborhood of a peak candidate fall into two categories:
  // 1. The bin has a lower value than the peak candidate.
  // 2. The bin has the same value as the peak candidate,
  // but it has a greater bin in its neighborhood.
  // The second case indicates that our candidate is not relly a peak.
  // To test for the second case we are going to increment the values
  // of the bins in the neighborhood of peak candidates, find the peak
  // candidates again and analize the differences.
  std::vector<unsigned> newHist(hist);
  incrementBinsMasked(newHist, width, height, neighborhoodMask);
  neighborhoodMask.release();

  BinaryImage diff(findPeakCandidates(newHist, width, height, lowerBound));
  rasterOp<RopXor<RopSrc, RopDst>>(diff, peakCandidates);

  // If a bin that has changed its state was a part of a peak candidate,
  // it means a neighboring bin went from equal to a greater value,
  // which indicates that such candidate is not a peak.
  const BinaryImage notPeaks(seedFill(diff, peakCandidates, CONN8));

  rasterOp<RopXor<RopSrc, RopDst>>(peakCandidates, notPeaks);

  return peakCandidates;
}  // HoughLineDetector::findHistogramPeaks

/**
 * Build a binary image where a black pixel indicates that the corresponding
 * histogram bin meets the following conditions:
 * \li It doesn't have a greater neighbor (in a 5x5 window).
 * \li It's value is not below \p lowerBound.
 */
BinaryImage HoughLineDetector::findPeakCandidates(const std::vector<unsigned>& hist,
                                                  const int width,
                                                  const int height,
                                                  const unsigned lowerBound) {
  std::vector<unsigned> maxed(hist.size(), 0);

  // Every bin becomes the maximum of itself and its neighbors.
  max5x5(hist, maxed, width, height);
  // Those that haven't changed didn't have a greater neighbor.
  BinaryImage equalMap(buildEqualMap(hist, maxed, width, height, lowerBound));

  return equalMap;
}

/**
 * Increment bins that correspond to black pixels in \p mask.
 */
void HoughLineDetector::incrementBinsMasked(std::vector<unsigned>& hist,
                                            const int width,
                                            const int height,
                                            const BinaryImage& mask) {
  const uint32_t* maskLine = mask.data();
  const int maskWpl = mask.wordsPerLine();
  unsigned* histLine = &hist[0];
  const uint32_t msb = uint32_t(1) << 31;

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      if (maskLine[x >> 5] & (msb >> (x & 31))) {
        ++histLine[x];
      }
    }
    maskLine += maskWpl;
    histLine += width;
  }
}

/**
 * Every bin in \p dst is set to the maximum of the corresponding bin
 * in \p src and its neighbors in a 5x5 window.
 */
void HoughLineDetector::max5x5(const std::vector<unsigned>& src,
                               std::vector<unsigned>& dst,
                               const int width,
                               const int height) {
  std::vector<unsigned> tmp(src.size(), 0);
  max3x1(src, tmp, width, height);
  max3x1(tmp, dst, width, height);
  max1x3(dst, tmp, width, height);
  max1x3(tmp, dst, width, height);
}

/**
 * Every bin in \p dst is set to the maximum of the corresponding bin
 * in \p src and its left and right neighbors (if it has them).
 */
void HoughLineDetector::max3x1(const std::vector<unsigned>& src,
                               std::vector<unsigned>& dst,
                               const int width,
                               const int height) {
  if (width == 1) {
    dst = src;

    return;
  }

  const unsigned* srcLine = &src[0];
  unsigned* dstLine = &dst[0];

  for (int y = 0; y < height; ++y) {
    // First column (no left neighbors).
    int x = 0;
    dstLine[x] = std::max(srcLine[x], srcLine[x + 1]);

    for (++x; x < width - 1; ++x) {
      const unsigned prev = srcLine[x - 1];
      const unsigned cur = srcLine[x];
      const unsigned next = srcLine[x + 1];
      dstLine[x] = std::max(prev, std::max(cur, next));
    }
    // Last column (no right neighbors).
    dstLine[x] = std::max(srcLine[x], srcLine[x - 1]);

    srcLine += width;
    dstLine += width;
  }
}

/**
 * Every bin in \p dst is set to the maximum of the corresponding bin
 * in \p src and its top and bottom neighbors (if it has them).
 */
void HoughLineDetector::max1x3(const std::vector<unsigned>& src,
                               std::vector<unsigned>& dst,
                               const int width,
                               const int height) {
  if (height == 1) {
    dst = src;

    return;
  }
  // First row (no top neighbors).
  const unsigned* pSrc = &src[0];
  unsigned* pDst = &dst[0];
  for (int x = 0; x < width; ++x) {
    *pDst = std::max(pSrc[0], pSrc[width]);
    ++pSrc;
    ++pDst;
  }

  for (int y = 1; y < height - 1; ++y) {
    for (int x = 0; x < width; ++x) {
      const unsigned prev = pSrc[x - width];
      const unsigned cur = pSrc[x];
      const unsigned next = pSrc[x + width];
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
}  // HoughLineDetector::max1x3

/**
 * Builds a binary image where a black pixel indicates that the corresponding
 * bins in two histograms have equal values, and those values are not below
 * \p lowerBound.
 */
BinaryImage HoughLineDetector::buildEqualMap(const std::vector<unsigned>& src1,
                                             const std::vector<unsigned>& src2,
                                             const int width,
                                             const int height,
                                             const unsigned lowerBound) {
  BinaryImage dst(width, height, WHITE);
  uint32_t* dstLine = dst.data();
  const int dstWpl = dst.wordsPerLine();
  const unsigned* src1Line = &src1[0];
  const unsigned* src2Line = &src2[0];
  const uint32_t msb = uint32_t(1) << 31;

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      if ((src1Line[x] >= lowerBound) && (src1Line[x] == src2Line[x])) {
        dstLine[x >> 5] |= msb >> (x & 31);
      }
    }
    dstLine += dstWpl;
    src1Line += width;
    src2Line += width;
  }

  return dst;
}

/*=============================== HoughLine ================================*/

QPointF HoughLine::pointAtY(const double y) const {
  double x = (m_distance - y * m_normUnitVector.y()) / m_normUnitVector.x();

  return QPointF(x, y);
}

QPointF HoughLine::pointAtX(const double x) const {
  double y = (m_distance - x * m_normUnitVector.x()) / m_normUnitVector.y();

  return QPointF(x, y);
}

QLineF HoughLine::unitSegment() const {
  const QPointF linePoint(m_normUnitVector * m_distance);
  const QPointF lineVector(m_normUnitVector.y(), -m_normUnitVector.x());

  return QLineF(linePoint, linePoint + lineVector);
}
}  // namespace imageproc