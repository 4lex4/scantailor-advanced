/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2007-2008  Joseph Artsimovich <joseph_a@mail.ru>

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

#include "Utils.h"
#include <QImage>
#include <QRect>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include "BinaryImage.h"
#include "Grayscale.h"

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
    const QRect left_of(0, 0, rect.x(), h);
    if (img1.copy(left_of) != img2.copy(left_of)) {
      return false;
    }
  }

  if (rect.right() != img1.rect().right()) {
    const QRect right_of(rect.x() + w, 0, w - rect.x() - rect.width(), h);
    if (img1.copy(right_of) != img2.copy(right_of)) {
      return false;
    }
  }

  if (rect.top() != 0) {
    const QRect top_of(0, 0, w, rect.y());
    if (img1.copy(top_of) != img2.copy(top_of)) {
      return false;
    }
  }

  if (rect.bottom() != img1.rect().bottom()) {
    const QRect bottom_of(0, rect.y() + rect.height(), w, h - rect.y() - rect.height());
    if (img1.copy(bottom_of) != img2.copy(bottom_of)) {
      return false;
    }
  }

  return true;
}  // surroundingsIntact
}  // namespace utils
}  // namespace tests
}  // namespace imageproc