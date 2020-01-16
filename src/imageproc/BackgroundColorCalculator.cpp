// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "BackgroundColorCalculator.h"

#include <cassert>

#include "Binarize.h"
#include "BinaryImage.h"
#include "Grayscale.h"
#include "Morphology.h"
#include "PolygonRasterizer.h"
#include "RasterOp.h"


namespace imageproc {
namespace {
class RgbHistogram {
 public:
  explicit RgbHistogram(const QImage& img);

  RgbHistogram(const QImage& img, const BinaryImage& mask);

  const int* redChannel() const { return m_red; }

  const int* greenChannel() const { return m_green; }

  const int* blueChannel() const { return m_blue; }

 private:
  void fromRgbImage(const QImage& img);

  void fromRgbImage(const QImage& img, const BinaryImage& mask);

  int m_red[256] = {0};
  int m_green[256] = {0};
  int m_blue[256] = {0};
};

RgbHistogram::RgbHistogram(const QImage& img) {
  if (img.isNull()) {
    return;
  }
  if (!((img.format() == QImage::Format_RGB32) || (img.format() == QImage::Format_ARGB32))) {
    throw std::invalid_argument("RgbHistogram: wrong image format");
  }

  fromRgbImage(img);
}

RgbHistogram::RgbHistogram(const QImage& img, const BinaryImage& mask) {
  if (img.isNull()) {
    return;
  }
  if (!((img.format() == QImage::Format_RGB32) || (img.format() == QImage::Format_ARGB32))) {
    throw std::invalid_argument("RgbHistogram: wrong image format");
  }
  if (img.size() != mask.size()) {
    throw std::invalid_argument("RgbHistogram: img and mask have different sizes");
  }

  fromRgbImage(img, mask);
}

void RgbHistogram::fromRgbImage(const QImage& img) {
  const auto* imgLine = reinterpret_cast<const uint32_t*>(img.bits());
  const int imgStride = img.bytesPerLine() / sizeof(uint32_t);

  const int width = img.width();
  const int height = img.height();

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      ++m_red[(imgLine[x] >> 16) & 0xff];
      ++m_green[(imgLine[x] >> 8) & 0xff];
      ++m_blue[imgLine[x] & 0xff];
    }
    imgLine += imgStride;
  }
}

void RgbHistogram::fromRgbImage(const QImage& img, const BinaryImage& mask) {
  const auto* imgLine = reinterpret_cast<const uint32_t*>(img.bits());
  const int imgStride = img.bytesPerLine() / sizeof(uint32_t);
  const uint32_t* maskLine = mask.data();
  const int maskStride = mask.wordsPerLine();

  const int width = img.width();
  const int height = img.height();
  const uint32_t msb = uint32_t(1) << 31;

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      if (maskLine[x >> 5] & (msb >> (x & 31))) {
        ++m_red[(imgLine[x] >> 16) & 0xff];
        ++m_green[(imgLine[x] >> 8) & 0xff];
        ++m_blue[imgLine[x] & 0xff];
      }
    }
    imgLine += imgStride;
    maskLine += maskStride;
  }
}

void grayHistToArray(int* rawHist, GrayscaleHistogram hist) {
  for (int i = 0; i < 256; ++i) {
    rawHist[i] = hist[i];
  }
}

void checkImageIsValid(const QImage& img) {
  if (!((img.format() == QImage::Format_RGB32) || (img.format() == QImage::Format_ARGB32)
        || ((img.format() == QImage::Format_Indexed8) && img.isGrayscale()))) {
    throw std::invalid_argument("BackgroundColorCalculator: wrong image format");
  }
  if (img.isNull()) {
    throw std::invalid_argument("BackgroundColorCalculator: image is null.");
  }
}
}  // namespace

uint8_t BackgroundColorCalculator::calcDominantLevel(const int* hist) {
  int integral_hist[256];
  integral_hist[0] = hist[0];
  for (int i = 1; i < 256; ++i) {
    integral_hist[i] = hist[i] + integral_hist[i - 1];
  }

  const int numColors = 256;
  const int windowSize = 10;

  int bestPos = 0;
  int bestSum = integral_hist[windowSize - 1];
  for (int i = 1; i <= numColors - windowSize; ++i) {
    const int sum = integral_hist[i + windowSize - 1] - integral_hist[i - 1];
    if (sum > bestSum) {
      bestSum = sum;
      bestPos = i;
    }
  }

  int halfSum = 0;
  for (int i = bestPos; i < bestPos + windowSize; ++i) {
    halfSum += hist[i];
    if (halfSum >= bestSum / 2) {
      return static_cast<uint8_t>(i);
    }
  }

  assert(!"Unreachable");
  return 0;
}  // BackgroundColorCalculator::calcDominantLevel

QColor BackgroundColorCalculator::calcDominantBackgroundColor(const QImage& img) {
  checkImageIsValid(img);

  BinaryImage backgroundMask(img, BinaryThreshold::otsuThreshold(img));
  backgroundMask.invert();
  return calcDominantColor(img, backgroundMask);
}

QColor BackgroundColorCalculator::calcDominantBackgroundColor(const QImage& img, const BinaryImage& mask) {
  checkImageIsValid(img);
  if (img.size() != mask.size()) {
    throw std::invalid_argument("BackgroundColorCalculator: img and mask have different sizes");
  }

  BinaryImage backgroundMask(img, BinaryThreshold::otsuThreshold(GrayscaleHistogram(img, mask)));
  backgroundMask.invert();
  rasterOp<RopAnd<RopSrc, RopDst>>(backgroundMask, mask);
  return calcDominantColor(img, backgroundMask);
}

QColor BackgroundColorCalculator::calcDominantBackgroundColor(const QImage& img, const QPolygonF& cropArea) {
  checkImageIsValid(img);
  if (cropArea.intersected(QRectF(img.rect())).isEmpty()) {
    throw std::invalid_argument("BackgroundColorCalculator: the cropping area is wrong.");
  }

  BinaryImage mask(img.size(), BLACK);
  PolygonRasterizer::fillExcept(mask, WHITE, cropArea, Qt::WindingFill);
  return calcDominantBackgroundColor(img, mask);
}

QColor BackgroundColorCalculator::calcDominantColor(const QImage& img, const BinaryImage& backgroundMask) {
  if ((img.format() == QImage::Format_Indexed8) && img.isGrayscale()) {
    const GrayscaleHistogram hist(img, backgroundMask);
    int rawHist[256];
    grayHistToArray(rawHist, hist);
    uint8_t dominantGray = calcDominantLevel(rawHist);
    return QColor(dominantGray, dominantGray, dominantGray);
  } else {
    const RgbHistogram hist(img, backgroundMask);
    uint8_t dominantRed = calcDominantLevel(hist.redChannel());
    uint8_t dominantGreen = calcDominantLevel(hist.greenChannel());
    uint8_t dominantBlue = calcDominantLevel(hist.blueChannel());
    return QColor(dominantRed, dominantGreen, dominantBlue);
  }
}
}  // namespace imageproc