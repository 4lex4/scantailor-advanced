// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include <BWColor.h>
#include <BinaryImage.h>
#include <Connectivity.h>
#include <Grayscale.h>
#include <SeedFill.h>
#include <QImage>
#include <QPoint>
#include <QSize>
#include <boost/test/unit_test.hpp>
#include "Utils.h"

namespace imageproc {
namespace tests {
using namespace utils;

BOOST_AUTO_TEST_SUITE(SeedFillTestSuite)

BOOST_AUTO_TEST_CASE(test_regression_1) {
  int seed_data[70 * 2] = {0};
  int mask_data[70 * 2] = {0};

  seed_data[32] = 1;
  seed_data[64] = 1;

  mask_data[32] = 1;
  mask_data[64] = 1;
  mask_data[70 + 31] = 1;
  mask_data[70 + 63] = 1;

  const BinaryImage seed(makeBinaryImage(seed_data, 70, 2));
  const BinaryImage mask(makeBinaryImage(mask_data, 70, 2));
  BOOST_CHECK(seedFill(seed, mask, CONN8) == mask);
}

BOOST_AUTO_TEST_CASE(test_regression_2) {
  int seed_data[70 * 2] = {0};
  int mask_data[70 * 2] = {0};

  seed_data[32] = 1;
  seed_data[64] = 1;

  mask_data[31] = 1;
  mask_data[63] = 1;
  mask_data[70 + 31] = 1;
  mask_data[70 + 63] = 1;

  const BinaryImage seed(makeBinaryImage(seed_data, 70, 2));
  const BinaryImage mask(makeBinaryImage(mask_data, 70, 2));
  BOOST_CHECK(seedFill(seed, mask, CONN8) == BinaryImage(70, 2, WHITE));
}

BOOST_AUTO_TEST_CASE(test_regression_3) {
  static const int seed_data[] = {1, 1, 0, 1, 0, 0, 1, 0, 0, 0, 0, 1, 1, 0, 1, 1, 0, 0, 0, 0, 1, 1, 1, 0, 0};

  static const int mask_data[] = {0, 1, 0, 0, 1, 0, 1, 0, 0, 0, 1, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 1, 0, 0, 1};

  static const int fill_data[] = {0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0};

  const BinaryImage seed(makeBinaryImage(seed_data, 5, 5));
  const BinaryImage mask(makeBinaryImage(mask_data, 5, 5));
  const BinaryImage fill(makeBinaryImage(fill_data, 5, 5));

  BOOST_REQUIRE(seedFill(seed, mask, CONN8) == fill);
}

BOOST_AUTO_TEST_CASE(test_regression_4) {
  static const int seed_data[] = {0, 1, 1, 0, 0, 1, 0, 0, 1, 0, 1, 1, 0, 0, 0, 1, 0, 1, 1, 0, 0, 0, 1, 1, 1};

  static const int mask_data[] = {1, 1, 1, 0, 1, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 1, 1, 1, 1, 0, 1, 0, 1, 0, 1};

  static const int fill_data[] = {1, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 1, 1, 1, 1, 0, 1, 0, 1, 0, 1};

  const BinaryImage seed(makeBinaryImage(seed_data, 5, 5));
  const BinaryImage mask(makeBinaryImage(mask_data, 5, 5));
  const BinaryImage fill(makeBinaryImage(fill_data, 5, 5));
  BOOST_REQUIRE(seedFill(seed, mask, CONN4) == fill);
}

BOOST_AUTO_TEST_CASE(test_gray4_random) {
  for (int i = 0; i < 200; ++i) {
    const GrayImage seed(randomGrayImage(5, 5));
    const GrayImage mask(randomGrayImage(5, 5));
    const GrayImage fillNew(seedFillGray(seed, mask, CONN4));
    const GrayImage fillOld(seedFillGraySlow(seed, mask, CONN4));
    if (fillNew != fillOld) {
      BOOST_ERROR("fillNew != fillOld at iteration " << i);
      dumpGrayImage(seed, "seed");
      dumpGrayImage(mask, "mask");
      dumpGrayImage(fillOld, "fillOld");
      dumpGrayImage(fillNew, "fillNew");
      break;
    }
  }
}

BOOST_AUTO_TEST_CASE(test_gray8_random) {
  for (int i = 0; i < 200; ++i) {
    const GrayImage seed(randomGrayImage(5, 5));
    const GrayImage mask(randomGrayImage(5, 5));
    const GrayImage fillNew(seedFillGray(seed, mask, CONN8));
    const GrayImage fillOld(seedFillGraySlow(seed, mask, CONN8));
    if (fillNew != fillOld) {
      BOOST_ERROR("fillNew != fillOld at iteration " << i);
      dumpGrayImage(seed, "seed");
      dumpGrayImage(mask, "mask");
      dumpGrayImage(fillOld, "fillOld");
      dumpGrayImage(fillNew, "fillNew");
      break;
    }
  }
}

BOOST_AUTO_TEST_CASE(test_gray_vs_binary) {
  for (int i = 0; i < 200; ++i) {
    const BinaryImage binSeed(randomBinaryImage(5, 5));
    const BinaryImage binMask(randomBinaryImage(5, 5));
    const GrayImage graySeed(toGrayscale(binSeed.toQImage()));
    const GrayImage grayMask(toGrayscale(binMask.toQImage()));
    const BinaryImage fillBin4(seedFill(binSeed, binMask, CONN4));
    const BinaryImage fillBin8(seedFill(binSeed, binMask, CONN8));
    const GrayImage fillGray4(seedFillGray(graySeed, grayMask, CONN4));
    const GrayImage fillGray8(seedFillGray(graySeed, grayMask, CONN8));

    if (fillGray4 != GrayImage(fillBin4.toQImage())) {
      BOOST_ERROR("grayscale 4-fill != binary 4-fill at index " << i);
      dumpBinaryImage(binSeed, "seed");
      dumpBinaryImage(binMask, "mask");
      dumpBinaryImage(fillBin4, "bin_fill");
      dumpBinaryImage(BinaryImage(fillGray4), "gray_fill");
      break;
    }

    if (fillGray8 != GrayImage(fillBin8.toQImage())) {
      BOOST_ERROR("grayscale 8-fill != binary 8-fill at index " << i);
      dumpBinaryImage(binSeed, "seed");
      dumpBinaryImage(binMask, "mask");
      dumpBinaryImage(fillBin8, "bin_fill");
      dumpBinaryImage(BinaryImage(fillGray8), "gray_fill");
      break;
    }
  }
}

BOOST_AUTO_TEST_SUITE_END()
}  // namespace tests
}  // namespace imageproc