// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "Utils.h"

#include <BinaryImage.h>
#include <Grayscale.h>

#include <QImage>
#include <QRect>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <iostream>

namespace imageproc {
namespace tests {
namespace utils {
BinaryImage randomBinaryImage(const int width, const int height) {
  BinaryImage image(width, height);
  uint32_t* pword = image.data();
  uint32_t* const end = pword + image.height() * image.wordsPerLine();
  for (; pword != end; ++pword) {
    const uint32_t w1 = rand() % (1 << 16);
    const uint32_t w2 = rand() % (1 << 16);
    *pword = (w1 << 16) | w2;
  }
  return image;
}

QImage randomMonoQImage(const int width, const int height) {
  QImage image(width, height, QImage::Format_Mono);
  image.setColorCount(2);
  image.setColor(0, 0xffffffff);
  image.setColor(1, 0xff000000);
  auto* pword = (uint32_t*) image.bits();
  assert(image.bytesPerLine() % 4 == 0);
  uint32_t* const end = pword + image.height() * (image.bytesPerLine() / 4);
  for (; pword != end; ++pword) {
    const uint32_t w1 = rand() % (1 << 16);
    const uint32_t w2 = rand() % (1 << 16);
    *pword = (w1 << 16) | w2;
  }
  return image;
}

QImage randomGrayImage(int width, int height) {
  QImage img(width, height, QImage::Format_Indexed8);
  img.setColorTable(createGrayscalePalette());
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      img.setPixel(x, y, rand() % 10);
    }
  }
  return img;
}

BinaryImage makeBinaryImage(const int* data, const int width, const int height) {
  return BinaryImage(makeMonoQImage(data, width, height));
}

QImage makeMonoQImage(const int* data, const int width, const int height) {
  QImage img(width, height, QImage::Format_Mono);
  img.setColorCount(2);
  img.setColor(0, 0xffffffff);
  img.setColor(1, 0xff000000);
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      img.setPixel(x, y, data[y * width + x] ? 1 : 0);
    }
  }
  return img;
}

QImage makeGrayImage(const int* data, const int width, const int height) {
  QImage img(width, height, QImage::Format_Indexed8);
  img.setColorTable(createGrayscalePalette());
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      img.setPixel(x, y, data[y * width + x]);
    }
  }
  return img;
}

void dumpBinaryImage(const BinaryImage& img, const char* name) {
  if (name) {
    std::cout << name << " = ";
  }

  if (img.isNull()) {
    std::cout << "NULL image" << std::endl;
    return;
  }

  const int width = img.width();
  const int height = img.height();
  const uint32_t* line = img.data();
  const int wpl = img.wordsPerLine();

  std::cout << "{\n";
  for (int y = 0; y < height; ++y, line += wpl) {
    std::cout << "\t";
    for (int x = 0; x < width; ++x) {
      std::cout << ((line[x >> 5] >> (31 - (x & 31))) & 1) << ", ";
    }
    std::cout << "\n";
  }
  std::cout << "}" << std::endl;
}

void dumpGrayImage(const QImage& img, const char* name) {
  if (name) {
    std::cout << name << " = ";
  }

  if (img.isNull()) {
    std::cout << "NULL image" << std::endl;
    return;
  }
  if (img.format() != QImage::Format_Indexed8) {
    std::cout << "Not grayscale image" << std::endl;
  }

  const int width = img.width();
  const int height = img.height();

  std::cout << "{\n";
  for (int y = 0; y < height; ++y) {
    std::cout << "\t";
    for (int x = 0; x < width; ++x) {
      std::cout << img.pixelIndex(x, y) << ", ";
    }
    std::cout << "\n";
  }
  std::cout << "}" << std::endl;
}

bool surroundingsIntact(const QImage& img1, const QImage& img2, const QRect& rect) {
  assert(img1.size() == img2.size());

  const int w = img1.width();
  const int h = img1.height();

  if (rect.left() != 0) {
    const QRect leftOf(0, 0, rect.x(), h);
    if (img1.copy(leftOf) != img2.copy(leftOf)) {
      return false;
    }
  }

  if (rect.right() != img1.rect().right()) {
    const QRect rightOf(rect.x() + w, 0, w - rect.x() - rect.width(), h);
    if (img1.copy(rightOf) != img2.copy(rightOf)) {
      return false;
    }
  }

  if (rect.top() != 0) {
    const QRect topOf(0, 0, w, rect.y());
    if (img1.copy(topOf) != img2.copy(topOf)) {
      return false;
    }
  }

  if (rect.bottom() != img1.rect().bottom()) {
    const QRect bottomOf(0, rect.y() + rect.height(), w, h - rect.y() - rect.height());
    if (img1.copy(bottomOf) != img2.copy(bottomOf)) {
      return false;
    }
  }
  return true;
}  // surroundingsIntact
}  // namespace utils
}  // namespace tests
}  // namespace imageproc