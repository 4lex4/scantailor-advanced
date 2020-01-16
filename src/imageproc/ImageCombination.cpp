// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "ImageCombination.h"

#include <QImage>
#include <unordered_map>
#include <unordered_set>

#include "BinaryImage.h"

namespace imageproc {
namespace impl {
template <typename MixedPixel>
void combineImagesMono(QImage& mixedImage, const BinaryImage& foreground) {
  auto* mixedLine = reinterpret_cast<MixedPixel*>(mixedImage.bits());
  const int mixedStride = mixedImage.bytesPerLine() / sizeof(MixedPixel);
  const uint32_t* foregroundLine = foreground.data();
  const int foregroundStride = foreground.wordsPerLine();
  const int width = mixedImage.width();
  const int height = mixedImage.height();
  const uint32_t msb = uint32_t(1) << 31;

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      if (foregroundLine[x >> 5] & (msb >> (x & 31))) {
        uint32_t tmp = foregroundLine[x >> 5];
        tmp >>= (31 - (x & 31));
        tmp &= uint32_t(1);

        --tmp;
        tmp |= 0xff000000;
        mixedLine[x] = static_cast<MixedPixel>(tmp);
      }
    }
    mixedLine += mixedStride;
    foregroundLine += foregroundStride;
  }
}

template <typename MixedPixel>
void combineImagesMono(QImage& mixedImage, const BinaryImage& foreground, const BinaryImage& mask) {
  auto* mixedLine = reinterpret_cast<MixedPixel*>(mixedImage.bits());
  const int mixedStride = mixedImage.bytesPerLine() / sizeof(MixedPixel);
  const uint32_t* foregroundLine = foreground.data();
  const int foregroundStride = foreground.wordsPerLine();
  const uint32_t* maskLine = mask.data();
  const int maskStride = mask.wordsPerLine();
  const int width = mixedImage.width();
  const int height = mixedImage.height();
  const uint32_t msb = uint32_t(1) << 31;

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      if (maskLine[x >> 5] & (msb >> (x & 31))) {
        uint32_t tmp = foregroundLine[x >> 5];
        tmp >>= (31 - (x & 31));
        tmp &= uint32_t(1);

        --tmp;
        tmp |= 0xff000000;
        mixedLine[x] = static_cast<MixedPixel>(tmp);
      }
    }
    mixedLine += mixedStride;
    foregroundLine += foregroundStride;
    maskLine += maskStride;
  }
}

template <typename MixedPixel>
void combineImagesColor(QImage& mixedImage, const QImage& foreground) {
  auto* mixedLine = reinterpret_cast<MixedPixel*>(mixedImage.bits());
  const int mixedStride = mixedImage.bytesPerLine() / sizeof(MixedPixel);
  const auto* foregroundLine = reinterpret_cast<const MixedPixel*>(foreground.bits());
  const int foregroundStride = foreground.bytesPerLine() / sizeof(MixedPixel);
  const int width = mixedImage.width();
  const int height = mixedImage.height();
  const auto msb = uint32_t(0x00ffffff);

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      if ((foregroundLine[x] & msb) != msb) {
        mixedLine[x] = foregroundLine[x];
      }
    }
    mixedLine += mixedStride;
    foregroundLine += foregroundStride;
  }
}

template <typename MixedPixel, typename ForegroundPixel>
void combineImagesColor(QImage& mixedImage, const QImage& foreground);

template <>
void combineImagesColor<uint32_t, uint8_t>(QImage& mixedImage, const QImage& foreground) {
  auto* mixedLine = reinterpret_cast<uint32_t*>(mixedImage.bits());
  const int mixedStride = mixedImage.bytesPerLine() / sizeof(uint32_t);
  const auto* foregroundLine = foreground.bits();
  const int foregroundStride = foreground.bytesPerLine();
  const int width = mixedImage.width();
  const int height = mixedImage.height();
  const auto msb = uint32_t(0x00ffffff);

  const QVector<QRgb> foregroundPalette = foreground.colorTable();

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      uint32_t color = foregroundPalette[foregroundLine[x]];
      if ((color & msb) != msb) {
        mixedLine[x] = color;
      }
    }
    mixedLine += mixedStride;
    foregroundLine += foregroundStride;
  }
}

void mergePalettes(QVector<uint32_t>& mixedPalette, const QVector<uint32_t>& palette) {
  std::unordered_set<uint32_t> mixedColors(mixedPalette.begin(), mixedPalette.end());
  for (uint32_t color : palette) {
    if (mixedColors.find(color) == mixedColors.end()) {
      mixedPalette.push_back(color);
      mixedColors.insert(color);
    }
  }
}

template <>
void combineImagesColor<uint8_t, uint8_t>(QImage& mixedImage, const QImage& foreground) {
  auto* mixedLine = mixedImage.bits();
  const int mixedStride = mixedImage.bytesPerLine();
  const auto* foregroundLine = foreground.bits();
  const int foregroundStride = foreground.bytesPerLine();
  const int width = mixedImage.width();
  const int height = mixedImage.height();
  const auto msb = uint32_t(0x00ffffff);

  QVector<uint32_t> mixedPalette = mixedImage.colorTable();
  const QVector<uint32_t> foregroundPalette = foreground.colorTable();
  if (mixedPalette.size() < 256) {
    mergePalettes(mixedPalette, foregroundPalette);
    if (mixedPalette.size() > 256) {
      mixedPalette.resize(256);
    }
    mixedImage.setColorTable(mixedPalette);
  }

  std::unordered_map<uint32_t, uint8_t> colorToIndex;
  for (int i = 0; i < mixedPalette.size(); ++i) {
    colorToIndex[mixedPalette[i]] = static_cast<uint8_t>(i);
  }

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      uint32_t color = foregroundPalette[foregroundLine[x]];
      if ((color & msb) != msb) {
        mixedLine[x] = colorToIndex[color];
      }
    }
    mixedLine += mixedStride;
    foregroundLine += foregroundStride;
  }
}

template <typename MixedPixel>
void combineImagesColor(QImage& mixedImage, const QImage& foreground, const BinaryImage& mask) {
  auto* mixedLine = reinterpret_cast<MixedPixel*>(mixedImage.bits());
  const int mixedStride = mixedImage.bytesPerLine() / sizeof(MixedPixel);
  const auto* foregroundLine = reinterpret_cast<const MixedPixel*>(foreground.bits());
  const int foregroundStride = foreground.bytesPerLine() / sizeof(MixedPixel);
  const uint32_t* maskLine = mask.data();
  const int maskStride = mask.wordsPerLine();
  const int width = mixedImage.width();
  const int height = mixedImage.height();
  const uint32_t msb = uint32_t(1) << 31;

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      if (maskLine[x >> 5] & (msb >> (x & 31))) {
        mixedLine[x] = foregroundLine[x];
      }
    }
    mixedLine += mixedStride;
    foregroundLine += foregroundStride;
    maskLine += maskStride;
  }
}

template <typename MixedPixel, typename ForegroundPixel>
void combineImagesColor(QImage& mixedImage, const QImage& foreground, const BinaryImage& mask);

template <>
void combineImagesColor<uint32_t, uint8_t>(QImage& mixedImage, const QImage& foreground, const BinaryImage& mask) {
  auto* mixedLine = reinterpret_cast<uint32_t*>(mixedImage.bits());
  const int mixedStride = mixedImage.bytesPerLine() / sizeof(uint32_t);
  const auto* foregroundLine = foreground.bits();
  const int foregroundStride = foreground.bytesPerLine();
  const uint32_t* maskLine = mask.data();
  const int maskStride = mask.wordsPerLine();
  const int width = mixedImage.width();
  const int height = mixedImage.height();
  const uint32_t msb = uint32_t(1) << 31;

  const QVector<QRgb> foregroundPalette = foreground.colorTable();

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      if (maskLine[x >> 5] & (msb >> (x & 31))) {
        uint32_t color = foregroundPalette[foregroundLine[x]];
        mixedLine[x] = color;
      }
    }
    mixedLine += mixedStride;
    foregroundLine += foregroundStride;
    maskLine += maskStride;
  }
}

template <>
void combineImagesColor<uint8_t, uint8_t>(QImage& mixedImage, const QImage& foreground, const BinaryImage& mask) {
  auto* mixedLine = mixedImage.bits();
  const int mixedStride = mixedImage.bytesPerLine();
  const auto* foregroundLine = foreground.bits();
  const int foregroundStride = foreground.bytesPerLine();
  const uint32_t* maskLine = mask.data();
  const int maskStride = mask.wordsPerLine();
  const int width = mixedImage.width();
  const int height = mixedImage.height();
  const uint32_t msb = uint32_t(1) << 31;

  QVector<uint32_t> mixedPalette = mixedImage.colorTable();
  const QVector<uint32_t> foregroundPalette = foreground.colorTable();
  if (mixedPalette.size() < 256) {
    mergePalettes(mixedPalette, foregroundPalette);
    if (mixedPalette.size() > 256) {
      mixedPalette.resize(256);
    }
    mixedImage.setColorTable(mixedPalette);
  }

  std::unordered_map<uint32_t, uint8_t> colorToIndex;
  for (int i = 0; i < mixedPalette.size(); ++i) {
    colorToIndex[mixedPalette[i]] = static_cast<uint8_t>(i);
  }

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      if (maskLine[x >> 5] & (msb >> (x & 31))) {
        uint32_t color = foregroundPalette[foregroundLine[x]];
        mixedLine[x] = colorToIndex[color];
      }
    }
    mixedLine += mixedStride;
    foregroundLine += foregroundStride;
    maskLine += maskStride;
  }
}

template <typename MixedPixel>
void applyMask(QImage& image, const BinaryImage& bwMask, const BWColor fillingColor = WHITE) {
  auto* imageLine = reinterpret_cast<MixedPixel*>(image.bits());
  const int imageStride = image.bytesPerLine() / sizeof(MixedPixel);
  const uint32_t* bwMaskLine = bwMask.data();
  const int bwMaskStride = bwMask.wordsPerLine();
  const int width = image.width();
  const int height = image.height();
  const uint32_t msb = uint32_t(1) << 31;
  const auto fillingPixel = static_cast<MixedPixel>((fillingColor == WHITE) ? 0xffffffff : 0x00000000);

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      if (!(bwMaskLine[x >> 5] & (msb >> (x & 31)))) {
        imageLine[x] = fillingPixel;
      }
    }
    imageLine += imageStride;
    bwMaskLine += bwMaskStride;
  }
}
}  // namespace impl

namespace {
inline void checkImageFormatSupported(const QImage& image) {
  if ((image.format() != QImage::Format_Indexed8) && (image.format() != QImage::Format_RGB32)
      && (image.format() != QImage::Format_ARGB32)) {
    throw std::invalid_argument("ImageCombination: wrong image format.");
  }
}

template <typename T1, typename T2>
inline void checkImagesHaveEqualSize(const T1& image1, const T2& image2) {
  if (image1.size() != image2.size()) {
    throw std::invalid_argument("ImageCombination: images size don't match.");
  }
}

template <typename T1, typename T2, typename T3>
inline void checkImagesHaveEqualSize(const T1& image1, const T2& image2, const T3& image3) {
  if ((image1.size() != image2.size()) || (image1.size() != image3.size())) {
    throw std::invalid_argument("ImageCombination: images size don't match.");
  }
}
}  // namespace

namespace impl {
void combineImagesMono(QImage& mixedImage, const BinaryImage& foreground) {
  if (mixedImage.format() == QImage::Format_Indexed8) {
    combineImagesMono<uint8_t>(mixedImage, foreground);
  } else {
    combineImagesMono<uint32_t>(mixedImage, foreground);
  }
}

void combineImagesMono(QImage& mixedImage, const BinaryImage& foreground, const BinaryImage& mask) {
  if (mixedImage.format() == QImage::Format_Indexed8) {
    combineImagesMono<uint8_t>(mixedImage, foreground, mask);
  } else {
    combineImagesMono<uint32_t>(mixedImage, foreground, mask);
  }
}

void combineImagesColor(QImage& mixedImage, const QImage& foreground) {
  if (mixedImage.format() == QImage::Format_Indexed8) {
    if (mixedImage.isGrayscale() && foreground.isGrayscale()) {
      combineImagesColor<uint8_t>(mixedImage, foreground);
    } else {
      combineImagesColor<uint8_t, uint8_t>(mixedImage, foreground);
    }
  } else {
    if ((foreground.format() == QImage::Format_RGB32) || (foreground.format() == QImage::Format_ARGB32)) {
      combineImagesColor<uint32_t>(mixedImage, foreground);
    } else {
      combineImagesColor<uint32_t, uint8_t>(mixedImage, foreground);
    }
  }
}

void combineImagesColor(QImage& mixedImage, const QImage& foreground, const BinaryImage& mask) {
  if (mixedImage.format() == QImage::Format_Indexed8) {
    if (mixedImage.isGrayscale() && foreground.isGrayscale()) {
      combineImagesColor<uint8_t>(mixedImage, foreground, mask);
    } else {
      combineImagesColor<uint8_t, uint8_t>(mixedImage, foreground, mask);
    }
  } else {
    if ((foreground.format() == QImage::Format_RGB32) || (foreground.format() == QImage::Format_ARGB32)) {
      combineImagesColor<uint32_t>(mixedImage, foreground, mask);
    } else {
      combineImagesColor<uint32_t, uint8_t>(mixedImage, foreground, mask);
    }
  }
}

void applyMask(QImage& image, const BinaryImage& bwMask, const BWColor fillingColor) {
  if (image.format() == QImage::Format_Indexed8) {
    applyMask<uint8_t>(image, bwMask, fillingColor);
  } else {
    applyMask<uint32_t>(image, bwMask, fillingColor);
  }
}
}  // namespace impl

void combineImages(QImage& mixedImage, const BinaryImage& foreground) {
  checkImageFormatSupported(mixedImage);
  checkImagesHaveEqualSize(mixedImage, foreground);

  impl::combineImagesMono(mixedImage, foreground);
}

void combineImages(QImage& mixedImage, const BinaryImage& foreground, const BinaryImage& mask) {
  checkImageFormatSupported(mixedImage);
  checkImagesHaveEqualSize(mixedImage, foreground);

  impl::combineImagesMono(mixedImage, foreground, mask);
}

void combineImages(QImage& mixedImage, const QImage& foreground) {
  checkImageFormatSupported(mixedImage);
  checkImagesHaveEqualSize(mixedImage, foreground);

  if ((foreground.format() == QImage::Format_Mono) || (foreground.format() == QImage::Format_MonoLSB)) {
    impl::combineImagesMono(mixedImage, BinaryImage(foreground));
  } else {
    impl::combineImagesColor(mixedImage, foreground);
  }
}

void combineImages(QImage& mixedImage, const QImage& foreground, const BinaryImage& mask) {
  checkImageFormatSupported(mixedImage);
  checkImagesHaveEqualSize(mixedImage, foreground, mask);

  if ((foreground.format() == QImage::Format_Mono) || (foreground.format() == QImage::Format_MonoLSB)) {
    impl::combineImagesMono(mixedImage, BinaryImage(foreground), mask);
  } else {
    impl::combineImagesColor(mixedImage, foreground, mask);
  }
}

void applyMask(QImage& image, const BinaryImage& bwMask, const BWColor fillingColor) {
  checkImageFormatSupported(image);
  checkImagesHaveEqualSize(image, bwMask);

  impl::applyMask(image, bwMask, fillingColor);
}
}  // namespace imageproc