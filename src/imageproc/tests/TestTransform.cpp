// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include <Grayscale.h>
#include <Transform.h>

#include <QImage>
#include <QSize>
#include <boost/test/unit_test.hpp>
#include <cmath>
#include <cstdint>
#include <cstdlib>

#include "Utils.h"

namespace imageproc {
namespace tests {
using namespace utils;

BOOST_AUTO_TEST_SUITE(TransformTestSuite)

BOOST_AUTO_TEST_CASE(test_null_image) {
  const QImage nullImg;
  const QTransform nullXform;
  const QRect unitRect(0, 0, 1, 1);
  const QColor bgcolor(0xff, 0xff, 0xff);
  const OutsidePixels outsidePixels(OutsidePixels::assumeColor(bgcolor));
  BOOST_CHECK(transformToGray(nullImg, nullXform, unitRect, outsidePixels).isNull());
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
  const OutsidePixels outsidePixels(OutsidePixels::assumeColor(bgcolor));

  const QTransform nullXform;
  BOOST_CHECK(transformToGray(img, nullXform, img.rect(), outsidePixels) == img);
}

BOOST_AUTO_TEST_SUITE_END()
}  // namespace tests
}  // namespace imageproc