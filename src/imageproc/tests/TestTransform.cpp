// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include <QImage>
#include <QSize>
#include <boost/test/auto_unit_test.hpp>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <Grayscale.h>
#include <Transform.h>
#include "Utils.h"

namespace imageproc {
namespace tests {
using namespace utils;

BOOST_AUTO_TEST_SUITE(TransformTestSuite)

BOOST_AUTO_TEST_CASE(test_null_image) {
  const QImage null_img;
  const QTransform null_xform;
  const QRect unit_rect(0, 0, 1, 1);
  const QColor bgcolor(0xff, 0xff, 0xff);
  const OutsidePixels outside_pixels(OutsidePixels::assumeColor(bgcolor));
  BOOST_CHECK(transformToGray(null_img, null_xform, unit_rect, outside_pixels).isNull());
}

BOOST_AUTO_TEST_CASE(test_random_image) {
  GrayImage img(QSize(100, 100));
  uint8_t* line = img.data();
  for (int y = 0; y < img.height(); ++y) {
    for (int x = 0; x < img.width(); ++x) {
      line[x] = static_cast<uint8_t>(rand() % 256);
    }
    line += img.stride();
  }

  const QColor bgcolor(0xff, 0xff, 0xff);
  const OutsidePixels outside_pixels(OutsidePixels::assumeColor(bgcolor));

  const QTransform null_xform;
  BOOST_CHECK(transformToGray(img, null_xform, img.rect(), outside_pixels) == img);
}

BOOST_AUTO_TEST_SUITE_END()
}  // namespace tests
}  // namespace imageproc