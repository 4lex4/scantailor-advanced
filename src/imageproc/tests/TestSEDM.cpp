// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include <BWColor.h>
#include <BinaryImage.h>
#include <SEDM.h>
#include <QImage>
#include <boost/test/auto_unit_test.hpp>
#include <cmath>
#include <iostream>
#include "Utils.h"

namespace imageproc {
namespace tests {
using namespace utils;

BOOST_AUTO_TEST_SUITE(SEDMTestSuite)

bool verifySEDM(const SEDM& sedm, const uint32_t* control) {
  const uint32_t* line = sedm.data();
  for (int y = 0; y < sedm.size().height(); ++y) {
    for (int x = 0; x < sedm.size().width(); ++x) {
      if (line[x] != *control) {
        return false;
      }
      ++control;
    }
    line += sedm.stride();
  }

  return true;
}

void dumpMatrix(const uint32_t* data, QSize size) {
  const int width = size.width();
  const int height = size.height();
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x, ++data) {
      std::cout << *data << ' ';
    }
    std::cout << std::endl;
  }
}

BOOST_AUTO_TEST_CASE(test1) {
  static const int inp[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0,
                            0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0,
                            0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

  static const uint32_t out[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0,
                                 0, 0, 1, 4, 4, 4, 1, 0, 0, 0, 0, 1, 4, 9, 4, 1, 0, 0, 0, 0, 1, 4, 4, 4, 1, 0, 0,
                                 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

  const BinaryImage img(makeBinaryImage(inp, 9, 9));
  const SEDM sedm(img, SEDM::DIST_TO_WHITE, SEDM::DIST_TO_NO_BORDERS);
  BOOST_CHECK(verifySEDM(sedm, out));
}

BOOST_AUTO_TEST_SUITE_END()
}  // namespace tests
}  // namespace imageproc