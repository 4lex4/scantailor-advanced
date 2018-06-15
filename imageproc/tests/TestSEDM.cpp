/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2007-2009  Joseph Artsimovich <joseph_a@mail.ru>

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

#include <QImage>
#include <boost/test/auto_unit_test.hpp>
#include <cmath>
#include <iostream>
#include "BWColor.h"
#include "BinaryImage.h"
#include "SEDM.h"
#include "Utils.h"

namespace imageproc {
namespace tests {
using namespace utils;

BOOST_AUTO_TEST_SUITE(SEDMTestSuite);

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

BOOST_AUTO_TEST_SUITE_END();
}  // namespace tests
}  // namespace imageproc