// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include <BinaryImage.h>
#include <ReduceThreshold.h>
#include <QImage>
#include <boost/test/auto_unit_test.hpp>
#include "Utils.h"

namespace imageproc {
namespace tests {
using namespace utils;

BOOST_AUTO_TEST_SUITE(ReduceThresholdTestSuite)

BOOST_AUTO_TEST_CASE(test_null_image) {
  BOOST_CHECK(ReduceThreshold(BinaryImage())(2).image().isNull());
}

BOOST_AUTO_TEST_CASE(test_small_image) {
  const int inp[] = {0, 0, 1, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

                     0, 0, 1, 0, 0, 1, 1, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0,

                     0, 0, 1, 0, 0, 1, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1,

                     0, 0, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
  const int out1[] = {0, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
  const int out2[] = {0, 0, 0, 1, 0, 0, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1};
  const int out3[] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 1, 1, 1, 1};
  const int out4[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1};

  const BinaryImage img(makeBinaryImage(inp, 10, 8));

  BOOST_CHECK(makeBinaryImage(out1, 5, 4) == ReduceThreshold(img)(1));
  BOOST_CHECK(makeBinaryImage(out2, 5, 4) == ReduceThreshold(img)(2));
  BOOST_CHECK(makeBinaryImage(out3, 5, 4) == ReduceThreshold(img)(3));
  BOOST_CHECK(makeBinaryImage(out4, 5, 4) == ReduceThreshold(img)(4));
}

BOOST_AUTO_TEST_CASE(test_lines) {
  static const int inp[] = {0, 0, 0, 1, 1, 0, 1, 1, 0};
  static const int out1[] = {0, 1, 1, 1};
  static const int out2[] = {0, 1, 1, 1};
  static const int out3[] = {0, 0, 0, 1};
  static const int out4[] = {0, 0, 0, 1};

  BinaryImage img(makeBinaryImage(inp, 9, 1));

  BOOST_CHECK(makeBinaryImage(out1, 4, 1) == ReduceThreshold(img)(1));
  BOOST_CHECK(makeBinaryImage(out2, 4, 1) == ReduceThreshold(img)(2));
  BOOST_CHECK(makeBinaryImage(out3, 4, 1) == ReduceThreshold(img)(3));
  BOOST_CHECK(makeBinaryImage(out4, 4, 1) == ReduceThreshold(img)(4));

  img = makeBinaryImage(inp, 1, 9);

  BOOST_CHECK(makeBinaryImage(out1, 1, 4) == ReduceThreshold(img)(1));
  BOOST_CHECK(makeBinaryImage(out2, 1, 4) == ReduceThreshold(img)(2));
  BOOST_CHECK(makeBinaryImage(out3, 1, 4) == ReduceThreshold(img)(3));
  BOOST_CHECK(makeBinaryImage(out4, 1, 4) == ReduceThreshold(img)(4));
}

BOOST_AUTO_TEST_SUITE_END()
}  // namespace tests
}  // namespace imageproc