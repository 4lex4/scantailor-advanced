/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2007-2008  Joseph Artsimovich <joseph_a@mail.ru>

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
#include <QSize>
#include <boost/test/auto_unit_test.hpp>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include "Grayscale.h"
#include "Transform.h"
#include "Utils.h"

namespace imageproc {
namespace tests {
using namespace utils;

BOOST_AUTO_TEST_SUITE(TransformTestSuite);

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

BOOST_AUTO_TEST_SUITE_END();
}  // namespace tests
}  // namespace imageproc