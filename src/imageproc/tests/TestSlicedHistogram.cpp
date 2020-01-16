// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include <BinaryImage.h>
#include <SlicedHistogram.h>

#include <QImage>
#include <boost/test/unit_test.hpp>
#include <cstddef>
#include <stdexcept>

#include "Utils.h"

namespace imageproc {
namespace tests {
using namespace utils;

static bool checkHistogram(const SlicedHistogram& hist, const int* dataBegin, const int* dataEnd) {
  if (hist.size() != size_t(dataEnd - dataBegin)) {
    return false;
  }
  for (unsigned i = 0; i < hist.size(); ++i) {
    if (hist[i] != dataBegin[i]) {
      return false;
    }
  }
  return true;
}

BOOST_AUTO_TEST_SUITE(SlicedHistogramTestSuite)

BOOST_AUTO_TEST_CASE(test_null_image) {
  const BinaryImage nullImg;

  SlicedHistogram horHist(nullImg, SlicedHistogram::ROWS);
  BOOST_CHECK(horHist.size() == 0);

  SlicedHistogram verHist(nullImg, SlicedHistogram::COLS);
  BOOST_CHECK(verHist.size() == 0);
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

  SlicedHistogram horHist(img, SlicedHistogram::ROWS);
  BOOST_CHECK(checkHistogram(horHist, hor_counts, hor_counts + 9));

  SlicedHistogram verHist(img, SlicedHistogram::COLS);
  BOOST_CHECK(checkHistogram(verHist, ver_counts, ver_counts + 9));

  horHist = SlicedHistogram(img, img.rect().adjusted(0, 1, 0, 0), SlicedHistogram::ROWS);
  BOOST_CHECK(checkHistogram(horHist, hor_counts + 1, hor_counts + 9));

  verHist = SlicedHistogram(img, img.rect().adjusted(1, 0, 0, 0), SlicedHistogram::COLS);
  BOOST_CHECK(checkHistogram(verHist, ver_counts + 1, ver_counts + 9));
}

BOOST_AUTO_TEST_SUITE_END()
}  // namespace tests
}  // namespace imageproc