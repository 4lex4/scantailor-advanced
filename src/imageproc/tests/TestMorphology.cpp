// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include <BWColor.h>
#include <BinaryImage.h>
#include <GrayImage.h>
#include <Morphology.h>

#include <QImage>
#include <QPoint>
#include <QSize>
#include <boost/test/unit_test.hpp>

#include "Utils.h"

namespace imageproc {
namespace tests {
using namespace utils;

BOOST_AUTO_TEST_SUITE(MorphologyTestSuite)

BOOST_AUTO_TEST_CASE(test_dilate_1x1) {
  static const int inp[] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0,
                            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0,
                            0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

  const BinaryImage img(makeBinaryImage(inp, 9, 9));
  BOOST_CHECK(dilateBrick(img, QSize(1, 1), img.rect()) == img);
}

BOOST_AUTO_TEST_CASE(test_dilate_1x1_gray) {
  static const int inp[] = {0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 7, 0, 0, 0, 0, 0, 0, 0,
                            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 6, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0,
                            0, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

  const GrayImage img(makeGrayImage(inp, 9, 9));
  BOOST_CHECK(dilateGray(img, QSize(1, 1), img.rect()) == img);
}

BOOST_AUTO_TEST_CASE(test_dilate_1x1_shift_black) {
  static const int inp[] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0,
                            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0,
                            0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

  static const int out[] = {0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1,
                            0, 0, 0, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1,
                            0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};

  const BinaryImage img(makeBinaryImage(inp, 9, 9));
  const BinaryImage control(makeBinaryImage(out, 9, 9));

  BOOST_CHECK(dilateBrick(img, QSize(1, 1), img.rect().translated(2, 2), BLACK) == control);
}

BOOST_AUTO_TEST_CASE(test_dilate_1x1_shift_gray) {
  static const int inp[] = {0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 7, 0, 0, 0, 0, 0, 0, 0,
                            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 6, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0,
                            0, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

  static const int out[] = {0, 0, 0, 0, 0, 0, 0, 5, 5, 0, 0, 0, 0, 0, 0, 0, 5, 5, 6, 5, 0, 0, 0, 0, 0, 5, 5,
                            0, 0, 0, 0, 0, 3, 0, 5, 5, 0, 4, 0, 0, 0, 0, 0, 5, 5, 0, 0, 0, 0, 0, 0, 0, 5, 5,
                            0, 0, 0, 0, 0, 0, 0, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5};

  const GrayImage img(makeGrayImage(inp, 9, 9));
  const GrayImage control(makeGrayImage(out, 9, 9));
  BOOST_CHECK(dilateGray(img, QSize(1, 1), img.rect().translated(2, 2), 5) == control);
}

BOOST_AUTO_TEST_CASE(test_dilate_3x1_gray) {
  static const int inp[] = {9, 4, 2, 3, 9, 9, 9, 9, 1, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
                            9, 3, 9, 3, 9, 9, 9, 9, 9, 9, 9, 9, 2, 2, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 5, 2, 9,
                            9, 9, 9, 9, 9, 9, 2, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9};

  static const int out[] = {4, 2, 2, 2, 3, 9, 9, 1, 1, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
                            3, 3, 3, 3, 3, 9, 9, 9, 9, 9, 9, 2, 2, 2, 2, 9, 9, 9, 9, 9, 9, 9, 9, 5, 2, 2, 2,
                            9, 9, 9, 9, 9, 2, 2, 2, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9};

  const GrayImage img(makeGrayImage(inp, 9, 9));
  const GrayImage control(makeGrayImage(out, 9, 9));
  BOOST_CHECK(dilateGray(img, QSize(3, 1), img.rect()) == control);
}

BOOST_AUTO_TEST_CASE(test_dilate_1x3_gray) {
  static const int inp[] = {9, 4, 9, 9, 9, 9, 9, 9, 1, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 2, 9, 9, 9, 9, 9, 9, 9,
                            9, 3, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 2, 9, 9, 9, 9, 9, 9, 9, 9, 2, 9, 9, 5, 2, 9,
                            9, 9, 9, 2, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9};

  static const int out[] = {9, 4, 9, 9, 9, 9, 9, 9, 1, 9, 2, 9, 9, 9, 9, 9, 9, 1, 9, 2, 9, 9, 9, 9, 9, 9, 9,
                            9, 2, 9, 2, 9, 9, 9, 9, 9, 9, 3, 9, 2, 9, 9, 5, 2, 9, 9, 9, 9, 2, 9, 9, 5, 2, 9,
                            9, 9, 9, 2, 9, 9, 5, 2, 9, 9, 9, 9, 2, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9};

  const GrayImage img(makeGrayImage(inp, 9, 9));
  const GrayImage control(makeGrayImage(out, 9, 9));
  BOOST_CHECK(dilateGray(img, QSize(1, 3), img.rect()) == control);
}

BOOST_AUTO_TEST_CASE(test_dilate_1x20_gray) {
  static const int inp[] = {9, 4, 9, 9, 9, 9, 9, 9, 1, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 2, 9, 9, 9, 9, 9, 9, 9,
                            9, 3, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 2, 9, 9, 9, 9, 9, 9, 9, 9, 2, 9, 9, 5, 2, 9,
                            9, 9, 9, 2, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9};

  static const int out[] = {9, 2, 9, 2, 9, 9, 5, 2, 1, 9, 2, 9, 2, 9, 9, 5, 2, 1, 9, 2, 9, 2, 9, 9, 5, 2, 1,
                            9, 2, 9, 2, 9, 9, 5, 2, 1, 9, 2, 9, 2, 9, 9, 5, 2, 1, 9, 2, 9, 2, 9, 9, 5, 2, 1,
                            9, 2, 9, 2, 9, 9, 5, 2, 1, 9, 2, 9, 2, 9, 9, 5, 2, 1, 9, 2, 9, 2, 9, 9, 5, 2, 1};

  const GrayImage img(makeGrayImage(inp, 9, 9));
  const GrayImage control(makeGrayImage(out, 9, 9));
  BOOST_CHECK(dilateGray(img, QSize(1, 20), img.rect()) == control);
}

BOOST_AUTO_TEST_CASE(test_dilate_3x3_white) {
  static const int inp[] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0,
                            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0,
                            0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

  static const int out[] = {0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0,
                            1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 1, 0, 1, 1, 1,
                            0, 0, 1, 1, 1, 0, 1, 1, 1, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

  const BinaryImage img(makeBinaryImage(inp, 9, 9));
  const BinaryImage control(makeBinaryImage(out, 9, 9));

  BOOST_CHECK(dilateBrick(img, QSize(3, 3), img.rect(), WHITE) == control);
}

BOOST_AUTO_TEST_CASE(test_dilate_3x3_gray) {
  static const int inp[] = {9, 4, 9, 9, 9, 9, 9, 9, 1, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 2, 9, 9, 9, 9, 9, 9, 9,
                            9, 3, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 2, 9, 9, 9, 9, 9, 9, 9, 9, 2, 9, 9, 5, 2, 9,
                            9, 9, 9, 2, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9};

  static const int out[] = {4, 4, 4, 9, 9, 9, 9, 1, 1, 2, 2, 2, 9, 9, 9, 9, 1, 1, 2, 2, 2, 9, 9, 9, 9, 9, 9,
                            2, 2, 2, 2, 2, 9, 9, 9, 9, 3, 3, 2, 2, 2, 5, 2, 2, 2, 9, 9, 2, 2, 2, 5, 2, 2, 2,
                            9, 9, 2, 2, 2, 5, 2, 2, 2, 9, 9, 2, 2, 2, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9};

  const GrayImage img(makeGrayImage(inp, 9, 9));
  const GrayImage control(makeGrayImage(out, 9, 9));
  BOOST_CHECK(dilateGray(img, QSize(3, 3), img.rect()) == control);
}

BOOST_AUTO_TEST_CASE(test_dilate_3x3_gray_shrinked) {
  static const int inp[] = {9, 4, 9, 9, 9, 9, 9, 9, 1, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 2, 9, 9, 9, 9, 9, 9, 9,
                            9, 3, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 2, 9, 9, 9, 9, 9, 9, 9, 9, 2, 9, 9, 5, 2, 9,
                            9, 9, 9, 2, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9};

  static const int out[] = {2, 2, 9, 9, 9, 9, 1, 2, 2, 9, 9, 9, 9, 9, 2, 2, 2, 2, 9, 9, 9, 3, 2, 2, 2,
                            5, 2, 2, 9, 2, 2, 2, 5, 2, 2, 9, 2, 2, 2, 5, 2, 2, 9, 2, 2, 2, 9, 9, 9};

  const GrayImage img(makeGrayImage(inp, 9, 9));
  const GrayImage control(makeGrayImage(out, 7, 7));
  const QRect dstArea(img.rect().adjusted(1, 1, -1, -1));
  BOOST_CHECK(dilateGray(img, QSize(3, 3), dstArea) == control);
}

BOOST_AUTO_TEST_CASE(test_open_1x2_gray) {
  static const int inp[] = {9, 4, 9, 9, 9, 9, 9, 9, 1, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 2, 9, 9, 9, 9, 9, 9, 9,
                            9, 3, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 2, 9, 9, 9, 9, 9, 9, 9, 9, 2, 9, 9, 5, 2, 9,
                            9, 9, 9, 2, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 1, 2, 3, 4, 5, 6, 7, 8, 9};

  static const int out[] = {9, 4, 9, 9, 9, 9, 9, 9, 1, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 3, 9, 9, 9, 9, 9, 9, 9,
                            9, 3, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 2, 9, 9, 9, 9, 9, 9, 9, 9, 2, 9, 9, 9, 9, 9,
                            9, 9, 9, 2, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 1, 2, 3, 4, 5, 6, 7, 8, 9};

  const GrayImage img(makeGrayImage(inp, 9, 9));
  const GrayImage control(makeGrayImage(out, 9, 9));
  BOOST_CHECK(openGray(img, QSize(1, 2), 0x00) == control);
}

BOOST_AUTO_TEST_CASE(test_dilate_5x5_white) {
  static const int inp[] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0,
                            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

  static const int out[] = {1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1,
                            1, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1,
                            0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 1, 1};

  const BinaryImage img(makeBinaryImage(inp, 9, 9));
  const BinaryImage control(makeBinaryImage(out, 9, 9));

  BOOST_CHECK(dilateBrick(img, QSize(5, 5), img.rect(), WHITE) == control);
}

BOOST_AUTO_TEST_CASE(test_dilate_3x3_narrowing_white) {
  static const int inp[] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0,
                            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0,
                            0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

  static const int out[]
      = {0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0};

  const BinaryImage img(makeBinaryImage(inp, 9, 9));
  const BinaryImage control(makeBinaryImage(out, 4, 9));
  const QRect dstRect(5, 0, 4, 9);

  BOOST_CHECK(dilateBrick(img, QSize(3, 3), dstRect, WHITE) == control);
}

BOOST_AUTO_TEST_CASE(test_dilate_5x5_narrowing_white) {
  static const int inp[]
      = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0,
         0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
         0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

  static const int out[] = {
      // 1, 1, 0, 0, 1, 1,
      1, 1, 0, 0, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 0, 0,
      0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1
      // 0, 0, 0, 1, 1, 1,
      // 0, 0, 0, 1, 1, 1,
      // 0, 0, 0, 1, 1, 1
  };

  const BinaryImage img(makeBinaryImage(inp, 11, 9));
  const BinaryImage control(makeBinaryImage(out, 6, 5));
  const QRect dstRect(4, 1, 6, 5);

  BOOST_CHECK(dilateBrick(img, QSize(5, 5), dstRect, WHITE) == control);
}

BOOST_AUTO_TEST_CASE(test_dilate_3x3_narrowing_black) {
  static const int inp[] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0,
                            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0,
                            0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

  static const int out[]
      = {1, 1, 1, 1, 0, 0, 1, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1};

  const BinaryImage img(makeBinaryImage(inp, 9, 9));
  const BinaryImage control(makeBinaryImage(out, 4, 9));
  const QRect dstRect(QRect(5, 0, 4, 9));

  BOOST_CHECK(dilateBrick(img, QSize(3, 3), dstRect, BLACK) == control);
}

BOOST_AUTO_TEST_CASE(test_dilate_3x3_widening_white) {
  static const int inp[] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0,
                            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0,
                            0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

  static const int out[] = {
      0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 1, 1, 1, 0, 0, 0, 0, 1,
      1, 1, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0,
      1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 0, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 0, 1, 1, 1, 0, 0, 0, 0, 1, 1,
      1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  };

  const BinaryImage img(makeBinaryImage(inp, 9, 9));
  const BinaryImage control(makeBinaryImage(out, 11, 11));
  const QRect dstRect(img.rect().adjusted(-1, -1, 1, 1));

  BOOST_CHECK(dilateBrick(img, QSize(3, 3), dstRect, WHITE) == control);
}

BOOST_AUTO_TEST_CASE(test_dilate_3x3_widening_black) {
  static const int inp[] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0,
                            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0,
                            0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

  static const int out[] = {
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1,
      1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0,
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 0, 1, 1,
      1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  };

  const BinaryImage img(makeBinaryImage(inp, 9, 9));
  const BinaryImage control(makeBinaryImage(out, 11, 11));
  const QRect dstRect(img.rect().adjusted(-1, -1, 1, 1));

  BOOST_CHECK(dilateBrick(img, QSize(3, 3), dstRect, BLACK) == control);
}

BOOST_AUTO_TEST_CASE(test_dilate_3x1_out_of_brick_white) {
  static const int inp[] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0,
                            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0,
                            0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

  static const int out[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0,
                            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
                            0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

  const BinaryImage img(makeBinaryImage(inp, 9, 9));
  const BinaryImage control(makeBinaryImage(out, 9, 9));
  const Brick brick(QSize(3, 1), QPoint(-1, 0));

  BOOST_CHECK(dilateBrick(img, brick, img.rect(), WHITE) == control);
}

BOOST_AUTO_TEST_CASE(test_dilate_1x3_out_of_brick_black) {
  static const int inp[] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0,
                            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0,
                            0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

  static const int out[] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                            0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0,
                            0, 0, 1, 1, 0, 0, 0, 1, 0, 0, 0, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0};

  const BinaryImage img(makeBinaryImage(inp, 9, 9));
  const BinaryImage control(makeBinaryImage(out, 9, 9));
  const Brick brick(QSize(1, 3), QPoint(0, -1));

  BOOST_CHECK(dilateBrick(img, brick, img.rect(), BLACK) == control);
}

BOOST_AUTO_TEST_CASE(test_large_dilate) {
  BinaryImage img(110, 110);
  img.fill(WHITE);
  const QRect initialRect(img.rect().center(), QSize(1, 1));
  img.fill(initialRect, BLACK);

  const Brick brick(QSize(80, 80));
  const QRect extendedRect(initialRect.adjusted(brick.minX(), brick.minY(), brick.maxX(), brick.maxY()));

  BinaryImage control(img);
  control.fill(extendedRect, BLACK);

  BOOST_CHECK(dilateBrick(img, brick, img.rect(), WHITE) == control);
}

BOOST_AUTO_TEST_CASE(test_erode_1x1) {
  static const int inp[] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1,
                            1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                            1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1};

  const BinaryImage img(makeBinaryImage(inp, 9, 9));
  BOOST_CHECK(erodeBrick(img, QSize(1, 1), img.rect()) == img);
}

BOOST_AUTO_TEST_CASE(test_erode_3x3_assymmetric_black) {
  static const int inp[] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1,
                            1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                            1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1};

  static const int out[] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1,
                            1, 1, 1, 1, 0, 0, 0, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0,
                            1, 1, 1, 1, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 1};

  const BinaryImage img(makeBinaryImage(inp, 9, 9));
  const BinaryImage control(makeBinaryImage(out, 9, 9));
  const Brick brick(QSize(3, 3), QPoint(0, 1));

  BOOST_CHECK(erodeBrick(img, brick, img.rect(), BLACK) == control);
}

BOOST_AUTO_TEST_CASE(test_erode_3x3_assymmetric_white) {
  static const int inp[] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1,
                            1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                            1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1};

  static const int out[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 0, 1, 1,
                            0, 0, 1, 1, 0, 0, 0, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0,
                            0, 0, 1, 1, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0};

  const BinaryImage img(makeBinaryImage(inp, 9, 9));
  const BinaryImage control(makeBinaryImage(out, 9, 9));
  const Brick brick(QSize(3, 3), QPoint(0, 1));

  BOOST_CHECK(erodeBrick(img, brick, img.rect(), WHITE) == control);
}

BOOST_AUTO_TEST_CASE(test_erode_11x11_white) {
  static const int inp[] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1,
                            1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                            1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1};

  static const int out[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

  const BinaryImage img(makeBinaryImage(inp, 9, 9));
  const BinaryImage control(makeBinaryImage(out, 9, 9));
  const Brick brick(QSize(11, 11));

  BOOST_CHECK(erodeBrick(img, brick, img.rect(), WHITE) == control);
}

BOOST_AUTO_TEST_CASE(test_open_2x2_white) {
  static const int inp[] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0,
                            0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0,
                            0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1};

  static const int out[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                            0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                            0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0};

  const BinaryImage img(makeBinaryImage(inp, 9, 9));
  const BinaryImage control(makeBinaryImage(out, 9, 9));

  BOOST_CHECK(openBrick(img, QSize(2, 2), img.rect(), WHITE) == control);
}

BOOST_AUTO_TEST_CASE(test_open_2x2_black) {
  static const int inp[] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0,
                            0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0,
                            0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1};

  static const int out[] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                            0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                            0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1};

  const BinaryImage img(makeBinaryImage(inp, 9, 9));
  const BinaryImage control(makeBinaryImage(out, 9, 9));

  BOOST_CHECK(openBrick(img, QSize(2, 2), img.rect(), BLACK) == control);
}

BOOST_AUTO_TEST_CASE(test_open_2x2_shifted_white) {
  static const int inp[] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0,
                            0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0,
                            0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1};

  static const int out[] = {// 0, 0, 0, 0, 0, 0, 0, 0, 0,
                            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0,
                            0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

  const BinaryImage img(makeBinaryImage(inp, 9, 9));
  const BinaryImage control(makeBinaryImage(out, 9, 9));
  const QRect dstRect(img.rect().translated(2, 1));

  BOOST_CHECK(openBrick(img, QSize(2, 2), dstRect, WHITE) == control);
}

BOOST_AUTO_TEST_CASE(test_open_2x2_shifted_black) {
  static const int inp[] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0,
                            0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0,
                            0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1};

  static const int out[] = {// 0, 0, 0, 0, 0, 0, 1, 1, 1
                            0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 1, 1, 0, 0, 1, 1,
                            0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1,
                            0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};

  const BinaryImage img(makeBinaryImage(inp, 9, 9));
  const BinaryImage control(makeBinaryImage(out, 9, 9));
  const QRect dstRect(img.rect().translated(2, 1));

  BOOST_CHECK(openBrick(img, QSize(2, 2), dstRect, BLACK) == control);
}

BOOST_AUTO_TEST_CASE(test_open_2x2_narrowing) {
  static const int inp[] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0,
                            0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0,
                            0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1};

  static const int out[] = {// 0, 0, 0, 0,
                            // 0, 0, 0, 0
                            0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0};

  const BinaryImage img(makeBinaryImage(inp, 9, 9));
  const BinaryImage control(makeBinaryImage(out, 4, 4));
  const QRect dstRect(img.rect().adjusted(2, 2, -3, -3));

  BOOST_CHECK(openBrick(img, QSize(2, 2), dstRect, WHITE) == control);
  BOOST_CHECK(openBrick(img, QSize(2, 2), dstRect, BLACK) == control);
}

BOOST_AUTO_TEST_CASE(test_close_2x2_white) {
  static const int inp[] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1,
                            1, 0, 0, 0, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 1,
                            0, 1, 0, 0, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 0};

  static const int out[] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1,
                            1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 0, 0, 1, 1, 1, 1, 1,
                            0, 1, 0, 0, 1, 1, 1, 1, 1, 0, 1, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 0};

  const BinaryImage img(makeBinaryImage(inp, 9, 9));
  const BinaryImage control(makeBinaryImage(out, 9, 9));

  BOOST_CHECK(closeBrick(img, QSize(2, 2), img.rect(), WHITE) == control);
}

BOOST_AUTO_TEST_CASE(test_close_2x2_black) {
  static const int inp[] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1,
                            1, 0, 0, 0, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 1,
                            0, 1, 0, 0, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 0};

  static const int out[] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1,
                            1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1,
                            1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1};

  const BinaryImage img(makeBinaryImage(inp, 9, 9));
  const BinaryImage control(makeBinaryImage(out, 9, 9));

  BOOST_CHECK(closeBrick(img, QSize(2, 2), img.rect(), BLACK) == control);
}

BOOST_AUTO_TEST_CASE(test_close_2x2_shifted_white) {
  static const int inp[] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1,
                            1, 0, 0, 0, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 1,
                            0, 1, 0, 0, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 0};

  static const int out[] = {// 1, 1, 1, 1, 1, 1, 1, 0, 0,
                            1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0,
                            1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0,
                            0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

  const BinaryImage img(makeBinaryImage(inp, 9, 9));
  const BinaryImage control(makeBinaryImage(out, 9, 9));
  const QRect dstRect(img.rect().translated(2, 1));

  BOOST_CHECK(closeBrick(img, QSize(2, 2), dstRect, WHITE) == control);
}

BOOST_AUTO_TEST_CASE(test_close_2x2_shifted_black) {
  static const int inp[] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1,
                            1, 0, 0, 0, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 1,
                            0, 1, 0, 0, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 0};

  static const int out[] = {// 1, 1, 1, 1, 1, 1, 1, 1, 1,
                            1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1,
                            1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1,
                            0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};

  const BinaryImage img(makeBinaryImage(inp, 9, 9));
  const BinaryImage control(makeBinaryImage(out, 9, 9));
  const QRect dstRect(img.rect().translated(2, 1));

  BOOST_CHECK(closeBrick(img, QSize(2, 2), dstRect, BLACK) == control);
}

BOOST_AUTO_TEST_CASE(test_close_2x2_narrowing) {
  static const int inp[] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1,
                            1, 0, 0, 0, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 1,
                            0, 1, 0, 0, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 0};

  static const int out[] = {// 0, 0, 0, 0,
                            // 0, 0, 0, 0
                            0, 0, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1};

  const BinaryImage img(makeBinaryImage(inp, 9, 9));
  const BinaryImage control(makeBinaryImage(out, 4, 4));
  const QRect dstRect(img.rect().adjusted(2, 2, -3, -3));

  BOOST_CHECK(closeBrick(img, QSize(2, 2), dstRect, WHITE) == control);
  BOOST_CHECK(closeBrick(img, QSize(2, 2), dstRect, BLACK) == control);
}

BOOST_AUTO_TEST_CASE(test_hmm_1) {
  static const int inp[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 1, 0, 0, 1, 0, 1, 0,
                            0, 1, 0, 1, 0, 1, 1, 1, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0,
                            0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0};

  static const int out[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0,
                            0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

  static const char pattern[]
      = "?X?"
        "X X"
        "?X?";
  const QPoint origin(1, 1);

  const BinaryImage img(makeBinaryImage(inp, 9, 9));
  const BinaryImage control(makeBinaryImage(out, 9, 9));

  BOOST_CHECK(hitMissMatch(img, WHITE, pattern, 3, 3, origin) == control);
  BOOST_CHECK(hitMissMatch(img, BLACK, pattern, 3, 3, origin) == control);
}

BOOST_AUTO_TEST_CASE(test_hmm_surroundings_1) {
  static const int inp[] = {0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0,
                            0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0,
                            0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0};

  static const int out_white[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

  static const int out_black[] = {0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

  static const char pattern[]
      = "?X?"
        "X X"
        "?X?";
  const QPoint origin(1, 1);

  const BinaryImage img(makeBinaryImage(inp, 9, 9));
  const BinaryImage controlW(makeBinaryImage(out_white, 9, 9));
  const BinaryImage controlB(makeBinaryImage(out_black, 9, 9));

  BOOST_CHECK(hitMissMatch(img, WHITE, pattern, 3, 3, origin) == controlW);
  BOOST_CHECK(hitMissMatch(img, BLACK, pattern, 3, 3, origin) == controlB);
}

BOOST_AUTO_TEST_CASE(test_hmm_surroundings_2) {
  static const int inp[] = {0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0,
                            0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0,
                            0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0};

  static const int out[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

  static const char pattern[]
      = "?X?"
        "X X"
        "?X?";
  const QPoint origin(1, 0);

  const BinaryImage img(makeBinaryImage(inp, 9, 9));
  const BinaryImage control(makeBinaryImage(out, 9, 9));

  BOOST_CHECK(hitMissMatch(img, WHITE, pattern, 3, 3, origin) == control);
  BOOST_CHECK(hitMissMatch(img, BLACK, pattern, 3, 3, origin) == control);
}

BOOST_AUTO_TEST_CASE(test_hmm_cornercase_1) {
  static const int inp[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 1, 0, 0, 1, 0, 1, 0,
                            0, 1, 0, 1, 0, 1, 1, 1, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0,
                            0, 0, 0, 0, 0, 1, 1, 1, 0, 1, 0, 0, 0, 0, 1, 1, 1, 0, 0, 1, 0, 0, 0, 1, 1, 1, 0};

  static const int out[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

  static const char pattern[]
      = "?X?"
        "X X"
        "?X?";
  const QPoint origin(10, 0);

  const BinaryImage img(makeBinaryImage(inp, 9, 9));
  const BinaryImage control(makeBinaryImage(out, 9, 9));

  BOOST_CHECK(hitMissMatch(img, WHITE, pattern, 3, 3, origin) == control);
  BOOST_CHECK(hitMissMatch(img, BLACK, pattern, 3, 3, origin) == control);
}

BOOST_AUTO_TEST_CASE(test_hmm_cornercase_2) {
  static const int inp[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 1, 0, 0, 1, 0, 1, 0,
                            0, 1, 0, 1, 0, 1, 1, 1, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0,
                            0, 0, 0, 0, 0, 1, 1, 1, 0, 1, 0, 0, 0, 0, 1, 1, 1, 0, 0, 1, 0, 0, 0, 1, 1, 1, 0};

  static const int out[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

  static const char pattern[]
      = "?X?"
        "X X"
        "?X?";
  const QPoint origin(0, 9);

  const BinaryImage img(makeBinaryImage(inp, 9, 9));
  const BinaryImage control(makeBinaryImage(out, 9, 9));

  BOOST_CHECK(hitMissMatch(img, WHITE, pattern, 3, 3, origin) == control);
  BOOST_CHECK(hitMissMatch(img, BLACK, pattern, 3, 3, origin) == control);
}

BOOST_AUTO_TEST_CASE(test_hmr_1) {
  static const int inp[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0,
                            0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 1, 1, 0, 1, 1, 0, 0, 0, 1, 0, 1, 1, 1, 1, 0,
                            0, 0, 1, 1, 0, 1, 1, 1, 0, 0, 0, 1, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0};

  static const int out[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0,
                            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 0, 1, 1, 1, 1, 0,
                            0, 0, 1, 1, 0, 1, 1, 1, 0, 0, 0, 1, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0};

  static const char pattern[]
      = " - "
        "X+X"
        "XXX";

  const BinaryImage img(makeBinaryImage(inp, 9, 9));
  const BinaryImage control(makeBinaryImage(out, 9, 9));

  BOOST_CHECK(hitMissReplace(img, WHITE, pattern, 3, 3) == control);
  BOOST_CHECK(hitMissReplace(img, BLACK, pattern, 3, 3) == control);
}

BOOST_AUTO_TEST_SUITE_END()
}  // namespace tests
}  // namespace imageproc