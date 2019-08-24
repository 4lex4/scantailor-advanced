// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include <BinaryImage.h>
#include <ConnComp.h>
#include <ConnCompEraser.h>
#include <QImage>
#include <algorithm>
#include <boost/test/auto_unit_test.hpp>
#include <list>
#include "Utils.h"

namespace imageproc {
namespace tests {
using namespace utils;

BOOST_AUTO_TEST_SUITE(ConnCompEraserTestSuite)

BOOST_AUTO_TEST_CASE(test_null_image) {
  ConnCompEraser eraser(BinaryImage(), CONN4);
  BOOST_CHECK(eraser.nextConnComp().isNull());
}

BOOST_AUTO_TEST_CASE(test_small_image) {
  static const int inp[]
      = {0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 1, 1, 1, 1, 0, 1, 1, 0, 1, 0, 0,
         0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0};

  std::list<QRect> c4r;
  c4r.emplace_back(2, 0, 3, 6);
  c4r.emplace_back(0, 3, 2, 1);
  c4r.emplace_back(1, 5, 1, 1);
  c4r.emplace_back(5, 2, 4, 3);
  c4r.emplace_back(0, 6, 7, 2);
  c4r.emplace_back(7, 6, 1, 1);

  std::list<QRect> c8r;
  c8r.emplace_back(0, 0, 9, 6);
  c8r.emplace_back(0, 6, 8, 2);

  BinaryImage img(makeBinaryImage(inp, 9, 8));

  ConnComp cc;
  ConnCompEraser eraser4(img, CONN4);
  while (!(cc = eraser4.nextConnComp()).isNull()) {
    const auto it(std::find(c4r.begin(), c4r.end(), cc.rect()));
    if (it != c4r.end()) {
      c4r.erase(it);
    } else {
      BOOST_ERROR("Incorrect 4-connected block found.");
    }
  }
  BOOST_CHECK_MESSAGE(c4r.empty(), "Not all 4-connected blocks were found.");

  ConnCompEraser eraser8(img, CONN8);
  while (!(cc = eraser8.nextConnComp()).isNull()) {
    const auto it(std::find(c8r.begin(), c8r.end(), cc.rect()));
    if (it != c8r.end()) {
      c8r.erase(it);
    } else {
      BOOST_ERROR("Incorrect 8-connected block found.");
    }
  }
  BOOST_CHECK_MESSAGE(c8r.empty(), "Not all 8-connected blocks were found.");
}

BOOST_AUTO_TEST_SUITE_END()
}  // namespace tests
}  // namespace imageproc