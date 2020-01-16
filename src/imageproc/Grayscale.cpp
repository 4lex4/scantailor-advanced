// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "Grayscale.h"

#include "BinaryImage.h"
#include "BitOps.h"

namespace imageproc {
static QImage monoMsbToGrayscale(const QImage& src) {
  const int width = src.width();
  const int height = src.height();

  QImage dst(width, height, QImage::Format_Indexed8);
  dst.setColorTable(createGrayscalePalette());
  if ((width > 0) && (height > 0) && dst.isNull()) {
    throw std::bad_alloc();
  }

  const uint8_t* srcLine = src.bits();
  uint8_t* dstLine = dst.bits();
  const int srcBpl = src.bytesPerLine();
  const int dstBpl = dst.bytesPerLine();

  uint8_t bin2gray[2] = {0, 0xff};
  if (src.colorCount() >= 2) {
    if (qGray(src.color(0)) > qGray(src.color(1))) {
      // if color 0 is lighter than color 1
      bin2gray[0] = 0xff;
      bin2gray[1] = 0;
    }
  }

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width;) {
      const uint8_t b = srcLine[x / 8];
      for (int i = 7; i >= 0 && x < width; --i, ++x) {
        dstLine[x] = bin2gray[(b >> i) & 1];
      }
    }

    srcLine += srcBpl;
    dstLine += dstBpl;
  }

  dst.setDotsPerMeterX(src.dotsPerMeterX());
  dst.setDotsPerMeterY(src.dotsPerMeterY());
  return dst;
}  // monoMsbToGrayscale

static QImage monoLsbToGrayscale(const QImage& src) {
  const int width = src.width();
  const int height = src.height();

  QImage dst(width, height, QImage::Format_Indexed8);
  dst.setColorTable(createGrayscalePalette());
  if ((width > 0) && (height > 0) && dst.isNull()) {
    throw std::bad_alloc();
  }

  const uint8_t* srcLine = src.bits();
  uint8_t* dstLine = dst.bits();
  const int srcBpl = src.bytesPerLine();
  const int dstBpl = dst.bytesPerLine();

  uint8_t bin2gray[2] = {0, 0xff};
  if (src.colorCount() >= 2) {
    if (qGray(src.color(0)) > qGray(src.color(1))) {
      // if color 0 is lighter than color 1
      bin2gray[0] = 0xff;
      bin2gray[1] = 0;
    }
  }

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width;) {
      const uint8_t b = srcLine[x / 8];
      for (int i = 0; i < 8 && x < width; ++i, ++x) {
        dstLine[x] = bin2gray[(b >> i) & 1];
      }
    }

    srcLine += srcBpl;
    dstLine += dstBpl;
  }

  dst.setDotsPerMeterX(src.dotsPerMeterX());
  dst.setDotsPerMeterY(src.dotsPerMeterY());
  return dst;
}  // monoLsbToGrayscale

static QImage anyToGrayscale(const QImage& src) {
  const int width = src.width();
  const int height = src.height();

  QImage dst(width, height, QImage::Format_Indexed8);
  dst.setColorTable(createGrayscalePalette());
  if ((width > 0) && (height > 0) && dst.isNull()) {
    throw std::bad_alloc();
  }

  uint8_t* dstLine = dst.bits();
  const int dstBpl = dst.bytesPerLine();

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      dstLine[x] = static_cast<uint8_t>(qGray(src.pixel(x, y)));
    }
    dstLine += dstBpl;
  }

  dst.setDotsPerMeterX(src.dotsPerMeterX());
  dst.setDotsPerMeterY(src.dotsPerMeterY());
  return dst;
}

QVector<QRgb> createGrayscalePalette() {
  QVector<QRgb> palette(256);
  for (int i = 0; i < 256; ++i) {
    palette[i] = qRgb(i, i, i);
  }
  return palette;
}

QImage toGrayscale(const QImage& src) {
  if (src.isNull()) {
    return src;
  }

  switch (src.format()) {
    case QImage::Format_Mono:
      return monoMsbToGrayscale(src);
    case QImage::Format_MonoLSB:
      return monoLsbToGrayscale(src);
    case QImage::Format_Indexed8:
      if (src.isGrayscale()) {
        if (src.colorCount() == 256) {
          return src;
        } else {
          QImage dst(src);
          dst.setColorTable(createGrayscalePalette());
          if (!src.isNull() && dst.isNull()) {
            throw std::bad_alloc();
          }
          return dst;
        }
      }
      // fall through
    default:
      return anyToGrayscale(src);
  }
}

GrayImage stretchGrayRange(const GrayImage& src, const double blackClipFraction, const double whiteClipFraction) {
  if (src.isNull()) {
    return src;
  }

  GrayImage dst(src);

  const int width = dst.width();
  const int height = dst.height();

  const int numPixels = width * height;
  int blackClipPixels = qRound(blackClipFraction * numPixels);
  int whiteClipPixels = qRound(whiteClipFraction * numPixels);

  const GrayscaleHistogram hist(dst);

  int min = 0;
  if (blackClipFraction >= 1.0) {
    min = qRound(blackClipFraction);
  } else {
    for (; min <= 255; ++min) {
      if (blackClipPixels < hist[min]) {
        break;
      }
      blackClipPixels -= hist[min];
    }
  }

  int max = 255;
  if (whiteClipFraction >= 1.0) {
    max = qRound(whiteClipFraction);
  } else {
    for (; max >= 0; --max) {
      if (whiteClipPixels < hist[max]) {
        break;
      }
      whiteClipPixels -= hist[max];
    }
  }

  uint8_t gray_mapping[256];

  if (min >= max) {
    const int avg = (min + max) / 2;
    for (int i = 0; i <= avg; ++i) {
      gray_mapping[i] = 0;
    }
    for (int i = avg + 1; i < 256; ++i) {
      gray_mapping[i] = 255;
    }
  } else {
    for (int i = 0; i < 256; ++i) {
      const int srcLevel = qBound(min, i, max);
      const int num = 255 * (srcLevel - min);
      const int denom = max - min;
      const int dstLevel = (num + denom / 2) / denom;
      gray_mapping[i] = static_cast<uint8_t>(dstLevel);
    }
  }

  uint8_t* line = dst.data();
  const int stride = dst.stride();

  for (int y = 0; y < height; ++y, line += stride) {
    for (int x = 0; x < width; ++x) {
      line[x] = gray_mapping[line[x]];
    }
  }
  return dst;
}  // stretchGrayRange

GrayImage createFramedImage(const QSize& size, const unsigned char innerColor, const unsigned char frameColor) {
  GrayImage image(size);
  image.fill(innerColor);

  const int width = size.width();
  const int height = size.height();

  unsigned char* line = image.data();
  const int stride = image.stride();

  memset(line, frameColor, width);

  for (int y = 0; y < height; ++y, line += stride) {
    line[0] = frameColor;
    line[width - 1] = frameColor;
  }

  memset(line - stride, frameColor, width);
  return image;
}

unsigned char darkestGrayLevel(const QImage& image) {
  const QImage gray(toGrayscale(image));

  const int width = image.width();
  const int height = image.height();

  const unsigned char* line = image.bits();
  const int bpl = image.bytesPerLine();

  unsigned char darkest = 0xff;

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      darkest = std::min(darkest, line[x]);
    }
    line += bpl;
  }
  return darkest;
}

GrayscaleHistogram::GrayscaleHistogram(const QImage& img) {
  if (img.isNull()) {
    return;
  }

  switch (img.format()) {
    case QImage::Format_Mono:
    case QImage::Format_MonoLSB:
      fromMonoImage(img);
      break;
    case QImage::Format_Indexed8:
      if (img.isGrayscale()) {
        fromGrayscaleImage(img);
        break;
      }
      // fall through
    default:
      fromAnyImage(img);
  }
}

GrayscaleHistogram::GrayscaleHistogram(const QImage& img, const BinaryImage& mask) {
  if (img.isNull()) {
    return;
  }

  if (img.size() != mask.size()) {
    throw std::invalid_argument("GrayscaleHistogram: img and mask have different sizes");
  }

  switch (img.format()) {
    case QImage::Format_Mono:
      fromMonoMSBImage(img, mask);
      break;
    case QImage::Format_MonoLSB:
      fromMonoMSBImage(img.convertToFormat(QImage::Format_Mono), mask);
      break;
    case QImage::Format_Indexed8:
      if (img.isGrayscale()) {
        fromGrayscaleImage(img, mask);
        break;
      }
      // fall through
    default:
      fromAnyImage(img, mask);
  }
}

void GrayscaleHistogram::fromMonoImage(const QImage& img) {
  const int w = img.width();
  const int h = img.height();
  const int bpl = img.bytesPerLine();
  const int lastByteIdx = (w - 1) >> 3;
  const int lastByteUnusedBits = (((lastByteIdx + 1) << 3) - w);
  uint8_t lastByteMask = ~uint8_t(0);
  if (img.format() == QImage::Format_MonoLSB) {
    lastByteMask >>= lastByteUnusedBits;
  } else {
    lastByteMask <<= lastByteUnusedBits;
  }
  const uint8_t* line = img.bits();

  int numBits1 = 0;
  for (int y = 0; y < h; ++y, line += bpl) {
    int i = 0;
    for (; i < lastByteIdx; ++i) {
      numBits1 += countNonZeroBits(line[i]);
    }

    // The last (possibly incomplete) byte.
    numBits1 += countNonZeroBits(line[i] & lastByteMask);
  }
  const int numBits0 = w * h - numBits1;

  QRgb color0 = 0xffffffff;
  QRgb color1 = 0xff000000;
  if (img.colorCount() >= 2) {
    color0 = img.color(0);
    color1 = img.color(1);
  }

  m_pixels[qGray(color0)] = numBits0;
  m_pixels[qGray(color1)] = numBits1;
}  // GrayscaleHistogram::fromMonoImage

void GrayscaleHistogram::fromMonoMSBImage(const QImage& img, const BinaryImage& mask) {
  const int w = img.width();
  const int h = img.height();
  const int wpl = img.bytesPerLine() >> 2;
  const int lastWordIdx = (w - 1) >> 5;
  const int lastWordUnusedBits = (((lastWordIdx + 1) << 5) - w);
  uint32_t lastWordMask = ~uint32_t(0) << lastWordUnusedBits;
  const auto* line = (const uint32_t*) img.bits();
  const uint32_t* maskLine = mask.data();
  const int maskWpl = mask.wordsPerLine();

  int numBits0 = 0;
  int numBits1 = 0;
  for (int y = 0; y < h; ++y, line += wpl, maskLine += maskWpl) {
    int i = 0;
    for (; i < lastWordIdx; ++i) {
      const uint32_t mask = maskLine[i];
      numBits1 += countNonZeroBits(line[i] & mask);
      numBits0 += countNonZeroBits(~line[i] & mask);
    }

    // The last (possibly incomplete) word.
    const uint32_t mask = maskLine[i] & lastWordMask;
    numBits1 += countNonZeroBits(line[i] & mask);
    numBits0 += countNonZeroBits(~line[i] & mask);
  }

  QRgb color0 = 0xffffffff;
  QRgb color1 = 0xff000000;
  if (img.colorCount() >= 2) {
    color0 = img.color(0);
    color1 = img.color(1);
  }

  m_pixels[qGray(color0)] = numBits0;
  m_pixels[qGray(color1)] = numBits1;
}  // GrayscaleHistogram::fromMonoMSBImage

void GrayscaleHistogram::fromGrayscaleImage(const QImage& img) {
  const int w = img.width();
  const int h = img.height();
  const int bpl = img.bytesPerLine();
  const uint8_t* line = img.bits();

  for (int y = 0; y < h; ++y, line += bpl) {
    for (int x = 0; x < w; ++x) {
      ++m_pixels[line[x]];
    }
  }
}

void GrayscaleHistogram::fromGrayscaleImage(const QImage& img, const BinaryImage& mask) {
  const int w = img.width();
  const int h = img.height();
  const int bpl = img.bytesPerLine();
  const uint8_t* line = img.bits();
  const uint32_t* maskLine = mask.data();
  const int maskWpl = mask.wordsPerLine();
  const uint32_t msb = uint32_t(1) << 31;

  for (int y = 0; y < h; ++y, line += bpl, maskLine += maskWpl) {
    for (int x = 0; x < w; ++x) {
      if (maskLine[x >> 5] & (msb >> (x & 31))) {
        ++m_pixels[line[x]];
      }
    }
  }
}

void GrayscaleHistogram::fromAnyImage(const QImage& img) {
  const int w = img.width();
  const int h = img.height();

  for (int y = 0; y < h; ++y) {
    for (int x = 0; x < w; ++x) {
      ++m_pixels[qGray(img.pixel(x, y))];
    }
  }
}

void GrayscaleHistogram::fromAnyImage(const QImage& img, const BinaryImage& mask) {
  const int w = img.width();
  const int h = img.height();
  const uint32_t* maskLine = mask.data();
  const int maskWpl = mask.wordsPerLine();
  const uint32_t msb = uint32_t(1) << 31;

  for (int y = 0; y < h; ++y, maskLine += maskWpl) {
    for (int x = 0; x < w; ++x) {
      if (maskLine[x >> 5] & (msb >> (x & 31))) {
        ++m_pixels[qGray(img.pixel(x, y))];
      }
    }
  }
}
}  // namespace imageproc