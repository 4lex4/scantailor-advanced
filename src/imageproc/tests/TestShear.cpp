// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include <QImage>
#include <boost/test/auto_unit_test.hpp>
#include <BWColor.h>
#include <BinaryImage.h>
#include <Shear.h>
#include "Utils.h"

namespace imageproc {
namespace tests {
using namespace utils;

BOOST_AUTO_TEST_SUITE(ShearTestSuite)

BOOST_AUTO_TEST_CASE(test_small_image) {
  static const int inp[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0,
                            0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0,
                            0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

  static const int h_out[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1,
                              0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0,
                              1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

  static const int v_out[] = {0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0,
                              0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0,
                              0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0};

  const BinaryImage img(makeBinaryImage(inp, 9, 9));
  const BinaryImage h_out_img(makeBinaryImage(h_out, 9, 9));
  const BinaryImage v_out_img(makeBinaryImage(v_out, 9, 9));

  const BinaryImage h_shear = hShear(img, -1.0, 0.5 * img.height(), WHITE);
  BOOST_REQUIRE(h_shear == h_out_img);

  const BinaryImage v_shear = vShear(img, 1.0, 0.5 * img.width(), WHITE);
  BOOST_REQUIRE(v_shear == v_out_img);

  BinaryImage h_shear_inplace(img);
  hShearInPlace(h_shear_inplace, -1.0, 0.5 * img.height(), WHITE);
  BOOST_REQUIRE(h_shear_inplace == h_out_img);

  BinaryImage v_shear_inplace(img);
  vShearInPlace(v_shear_inplace, 1.0, 0.5 * img.width(), WHITE);
  BOOST_REQUIRE(v_shear_inplace == v_out_img);
}

BOOST_AUTO_TEST_SUITE_END()
}  // namespace tests
}  // namespace imageproc