// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include <BWColor.h>
#include <BinaryImage.h>

#include <QImage>
#include <boost/test/unit_test.hpp>
#include <cstdlib>

#include "Utils.h"

namespace imageproc {
namespace tests {
using namespace utils;

BOOST_AUTO_TEST_SUITE(BinaryImageTestSuite)

BOOST_AUTO_TEST_CASE(test_null_image) {
  BOOST_CHECK(BinaryImage().toQImage() == QImage());
  BOOST_CHECK(BinaryImage(QImage()).toQImage() == QImage());
}

BOOST_AUTO_TEST_CASE(test_from_to_qimage) {
  const int w = 50;
  const int h = 64;
  QImage qimgArgb32(w, h, QImage::Format_ARGB32);
  QImage qimgMono(w, h, QImage::Format_Mono);
  qimgMono.setColorCount(2);
  qimgMono.setColor(0, 0xffffffff);
  qimgMono.setColor(1, 0xff000000);
  for (int y = 0; y < h; ++y) {
    for (int x = 0; x < w; ++x) {
      const int rnd = rand() & 1;
      qimgArgb32.setPixel(x, y, rnd ? 0x66888888 : 0x66777777);
      qimgMono.setPixel(x, y, rnd ? 0 : 1);
    }
  }

  QImage qimgMonoLsb(qimgMono.convertToFormat(QImage::Format_MonoLSB));
  QImage qimgRgb32(qimgArgb32.convertToFormat(QImage::Format_RGB32));
  QImage qimgArgb32Pm(qimgArgb32.convertToFormat(QImage::Format_ARGB32_Premultiplied));
  QImage qimgRgb16(qimgRgb32.convertToFormat(QImage::Format_RGB16));
  QImage qimgIndexed8(qimgRgb32.convertToFormat(QImage::Format_Indexed8));

  BOOST_REQUIRE(BinaryImage(qimgMono).toQImage() == qimgMono);
  BOOST_CHECK(BinaryImage(qimgMonoLsb).toQImage() == qimgMono);
  BOOST_CHECK(BinaryImage(qimgArgb32).toQImage() == qimgMono);
  BOOST_CHECK(BinaryImage(qimgRgb32).toQImage() == qimgMono);
  BOOST_CHECK(BinaryImage(qimgArgb32Pm).toQImage() == qimgMono);
  BOOST_CHECK(BinaryImage(qimgIndexed8).toQImage() == qimgMono);

  // A bug in Qt prevents this from working.
  // BOOST_CHECK(BinaryImage(qimgRgb16, 0x80).toQImage() == qimgMono);
}

BOOST_AUTO_TEST_CASE(test_full_fill) {
  BinaryImage white(100, 100);
  white.fill(WHITE);

  QImage qWhite(100, 100, QImage::Format_Mono);
  qWhite.fill(1);

  BOOST_REQUIRE(BinaryImage(qWhite) == white);

  BinaryImage black(30, 30);
  black.fill(BLACK);

  QImage qBlack(30, 30, QImage::Format_Mono);
  qBlack.fill(0);

  BOOST_REQUIRE(BinaryImage(qBlack) == black);
}

BOOST_AUTO_TEST_CASE(test_partial_fill_small) {
  QImage qImage(randomMonoQImage(100, 100));

  const QRect rect(80, 80, 20, 20);
  BinaryImage image(qImage);
  image.fill(rect, WHITE);
  QImage whiteRect(rect.width(), rect.height(), QImage::Format_Mono);
  whiteRect.setColorCount(2);
  whiteRect.setColor(0, 0xffffffff);
  whiteRect.setColor(1, 0xff000000);
  whiteRect.fill(0);
  BOOST_REQUIRE(image.toQImage().copy(rect) == whiteRect);
  BOOST_CHECK(surroundingsIntact(image.toQImage(), qImage, rect));
}

BOOST_AUTO_TEST_CASE(test_partial_fill_large) {
  QImage qImage(randomMonoQImage(100, 100));

  const QRect rect(20, 20, 79, 79);
  BinaryImage image(qImage);
  image.fill(rect, WHITE);
  QImage whiteRect(rect.width(), rect.height(), QImage::Format_Mono);
  whiteRect.setColorCount(2);
  whiteRect.setColor(0, 0xffffffff);
  whiteRect.setColor(1, 0xff000000);
  whiteRect.fill(0);
  BOOST_REQUIRE(image.toQImage().copy(rect) == whiteRect);
  BOOST_CHECK(surroundingsIntact(image.toQImage(), qImage, rect));
}

BOOST_AUTO_TEST_CASE(test_fill_except) {
  QImage qImage(randomMonoQImage(100, 100));

  const QRect rect(20, 20, 79, 79);
  BinaryImage image(qImage);
  image.fillExcept(rect, BLACK);

  QImage blackImage(qImage.width(), qImage.height(), QImage::Format_Mono);
  blackImage.setColorCount(2);
  blackImage.setColor(0, 0xffffffff);
  blackImage.setColor(1, 0xff000000);
  blackImage.fill(1);

  BOOST_REQUIRE(image.toQImage().copy(rect) == qImage.copy(rect));
  BOOST_CHECK(surroundingsIntact(image.toQImage(), blackImage, rect));
}

BOOST_AUTO_TEST_CASE(test_content_bounding_box4) {
  static const int inp[]
      = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0,
         0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

  const BinaryImage img(makeBinaryImage(inp, 9, 8));
  BOOST_CHECK(img.contentBoundingBox() == QRect(1, 1, 6, 6));
}

BOOST_AUTO_TEST_SUITE_END()
}  // namespace tests
}  // namespace imageproc