// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include <QImage>
#include <boost/test/auto_unit_test.hpp>
#include <cstddef>
#include <stdexcept>
#include <BinaryImage.h>
#include <SlicedHistogram.h>
#include "Utils.h"

namespace imageproc {
namespace tests {
using namespace utils;

static bool checkHistogram(const SlicedHistogram& hist, const int* data_begin, const int* data_end) {
  if (hist.size() != size_t(data_end - data_begin)) {
    return false;
  }
  for (unsigned i = 0; i < hist.size(); ++i) {
    if (hist[i] != data_begin[i]) {
      return false;
    }
  }

  return true;
}

BOOST_AUTO_TEST_SUITE(SlicedHistogramTestSuite)

BOOST_AUTO_TEST_CASE(test_null_image) {
  const BinaryImage null_img;

  SlicedHistogram hor_hist(null_img, SlicedHistogram::ROWS);
  BOOST_CHECK(hor_hist.size() == 0);

  SlicedHistogram ver_hist(null_img, SlicedHistogram::COLS);
  BOOST_CHECK(ver_hist.size() == 0);
}

BOOST_AUTO_TEST_CASE(test_exceeding_area) {
  const BinaryImage img(1, 1);
  const QRect area(0, 0, 1, 2);

  BOOST_CHECK_THROW(SlicedHistogram(img, area, SlicedHistogram::ROWS), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(test_small_image) {
  static const int inp[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0,
                            1, 0, 0, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, 0, 0, 1, 0,
                            0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1};

  static const int hor_counts[] = {0, 1, 2, 3, 9, 2, 6, 3, 1};

  static const int ver_counts[] = {2, 2, 4, 4, 5, 3, 2, 3, 2};

  const BinaryImage img(makeBinaryImage(inp, 9, 9));

  SlicedHistogram hor_hist(img, SlicedHistogram::ROWS);
  BOOST_CHECK(checkHistogram(hor_hist, hor_counts, hor_counts + 9));

  SlicedHistogram ver_hist(img, SlicedHistogram::COLS);
  BOOST_CHECK(checkHistogram(ver_hist, ver_counts, ver_counts + 9));

  hor_hist = SlicedHistogram(img, img.rect().adjusted(0, 1, 0, 0), SlicedHistogram::ROWS);
  BOOST_CHECK(checkHistogram(hor_hist, hor_counts + 1, hor_counts + 9));

  ver_hist = SlicedHistogram(img, img.rect().adjusted(1, 0, 0, 0), SlicedHistogram::COLS);
  BOOST_CHECK(checkHistogram(ver_hist, ver_counts + 1, ver_counts + 9));
}

BOOST_AUTO_TEST_SUITE_END()
}  // namespace tests
}  // namespace imageproc