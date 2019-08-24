// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "BinaryThreshold.h"
#include <QDebug>
#include <cassert>
#include <iostream>
#include "Grayscale.h"
#include "Morphology.h"

namespace imageproc {
BinaryThreshold BinaryThreshold::otsuThreshold(const QImage& image) {
  return otsuThreshold(GrayscaleHistogram(image));
}

BinaryThreshold BinaryThreshold::otsuThreshold(const GrayscaleHistogram& pixelsByColor) {
  int32_t pixels_by_threshold[256];
  int64_t moment_by_threshold[256];

  // Note that although BinaryThreshold is defined in such a way
  // that everything below the threshold is considered black,
  // this algorithm assumes that everything below *or equal* to
  // the threshold is considered black.
  // That is, pixels_by_threshold[10] holds the number of pixels
  // in the image that have a gray_level <= 10

  pixels_by_threshold[0] = pixelsByColor[0];
  moment_by_threshold[0] = 0;
  for (int i = 1; i < 256; ++i) {
    pixels_by_threshold[i] = pixels_by_threshold[i - 1] + pixelsByColor[i];
    moment_by_threshold[i] = moment_by_threshold[i - 1] + int64_t(pixelsByColor[i]) * i;
  }

  const int totalPixels = pixels_by_threshold[255];
  const int64_t totalMoment = moment_by_threshold[255];
  double maxVariance = 0.0;
  int firstBestThreshold = -1;
  int lastBestThreshold = -1;
  for (int i = 0; i < 256; ++i) {
    const int pixelsBelow = pixels_by_threshold[i];
    const int pixelsAbove = totalPixels - pixelsBelow;
    if ((pixelsBelow > 0) && (pixelsAbove > 0)) {  // prevent division by zero
      const int64_t momentBelow = moment_by_threshold[i];
      const int64_t momentAbove = totalMoment - momentBelow;
      const double meanBelow = (double) momentBelow / pixelsBelow;
      const double meanAbove = (double) momentAbove / pixelsAbove;
      const double meanDiff = meanBelow - meanAbove;
      const double variance = meanDiff * meanDiff * pixelsBelow * pixelsAbove;
      if (variance > maxVariance) {
        maxVariance = variance;
        firstBestThreshold = i;
        lastBestThreshold = i;
      } else if (variance == maxVariance) {
        lastBestThreshold = i;
      }
    }
  }

  // Compensate the "< threshold" vs "<= threshold" difference.
  ++firstBestThreshold;
  ++lastBestThreshold;

  // The middle between the two.
  return BinaryThreshold((firstBestThreshold + lastBestThreshold) >> 1);
}  // BinaryThreshold::otsuThreshold

BinaryThreshold BinaryThreshold::peakThreshold(const QImage& image) {
  return peakThreshold(GrayscaleHistogram(image));
}

BinaryThreshold BinaryThreshold::peakThreshold(const GrayscaleHistogram& pixelsByColor) {
  int ri = 255, li = 0;
  int rightPeak = pixelsByColor[ri];
  int leftPeak = pixelsByColor[li];

  for (int i = 254; i >= 0; --i) {
    if (pixelsByColor[i] <= rightPeak) {
      if (double(pixelsByColor[i]) < (double(rightPeak) * 0.66)) {
        break;
      }
      continue;
    }
    if (pixelsByColor[i] > rightPeak) {
      rightPeak = pixelsByColor[i];
      ri = i;
    }
  }

  for (int i = 1; i <= 255; ++i) {
    if (pixelsByColor[i] <= leftPeak) {
      if (double(pixelsByColor[i]) < (double(leftPeak) * 0.66)) {
        break;
      }
      continue;
    }
    if (pixelsByColor[i] > leftPeak) {
      leftPeak = pixelsByColor[i];
      li = i;
    }
  }

  auto threshold = static_cast<int>(li + (ri - li) * 0.75);

#ifdef DEBUG
  int otsuThreshold = BinaryThreshold::otsuThreshold(pixelsByColor);
  std::cout << "li: " << li << " leftPeak: " << leftPeak << std::endl;
  std::cout << "ri: " << ri << " rightPeak: " << rightPeak << std::endl;
  std::cout << "threshold: " << threshold << std::endl;
  std::cout << "otsuThreshold: " << otsuThreshold << std::endl;
#endif

  return BinaryThreshold(threshold);
}  // BinaryThreshold::peakThreshold

BinaryThreshold BinaryThreshold::mokjiThreshold(const QImage& image,
                                                const unsigned maxEdgeWidth,
                                                const unsigned minEdgeMagnitude) {
  if (maxEdgeWidth < 1) {
    throw std::invalid_argument("mokjiThreshold: invalud maxEdgeWidth");
  }
  if (minEdgeMagnitude < 1) {
    throw std::invalid_argument("mokjiThreshold: invalid minEdgeMagnitude");
  }

  const GrayImage gray(image);

  const int dilateSize = (maxEdgeWidth + 1) * 2 - 1;
  GrayImage dilated(dilateGray(gray, QSize(dilateSize, dilateSize)));

  unsigned matrix[256][256];
  memset(matrix, 0, sizeof(matrix));

  const int w = image.width();
  const int h = image.height();
  const unsigned char* srcLine = gray.data();
  const int srcStride = gray.stride();
  const unsigned char* dilatedLine = dilated.data();
  const int dilatedStride = dilated.stride();

  srcLine += maxEdgeWidth * srcStride;
  dilatedLine += maxEdgeWidth * dilatedStride;
  for (int y = maxEdgeWidth; y < h - (int) maxEdgeWidth; ++y) {
    for (int x = maxEdgeWidth; x < w - (int) maxEdgeWidth; ++x) {
      const unsigned pixel = srcLine[x];
      const unsigned darkestNeighbor = dilatedLine[x];
      assert(darkestNeighbor <= pixel);

      ++matrix[darkestNeighbor][pixel];
    }
    srcLine += srcStride;
    dilatedLine += dilatedStride;
  }

  unsigned nominator = 0;
  unsigned denominator = 0;
  for (unsigned m = 0; m < 256 - minEdgeMagnitude; ++m) {
    for (unsigned n = m + minEdgeMagnitude; n < 256; ++n) {
      assert(n >= m);

      const unsigned val = matrix[m][n];
      nominator += (m + n) * val;
      denominator += val;
    }
  }

  if (denominator == 0) {
    return BinaryThreshold(128);
  }

  const double threshold = 0.5 * nominator / denominator;

  return BinaryThreshold((int) (threshold + 0.5));
}  // BinaryThreshold::mokjiThreshold
}  // namespace imageproc