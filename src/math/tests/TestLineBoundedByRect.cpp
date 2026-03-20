// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include <LineBoundedByRect.h>

#include <QLineF>
#include <QRectF>
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(MathLineBoundedByRectTestSuite)

BOOST_AUTO_TEST_CASE(test_line_through_rect_center) {
  const QRectF rect(0, 0, 100, 100);
  QLineF line(-10, 50, 110, 50);
  const bool ok = lineBoundedByRect(line, rect);
  BOOST_REQUIRE(ok);
  BOOST_CHECK_CLOSE(line.p1().x(), 0.0, 0.01);
  BOOST_CHECK_CLOSE(line.p1().y(), 50.0, 0.01);
  BOOST_CHECK_CLOSE(line.p2().x(), 100.0, 0.01);
  BOOST_CHECK_CLOSE(line.p2().y(), 50.0, 0.01);
}

BOOST_AUTO_TEST_CASE(test_line_through_rect_diagonal) {
  const QRectF rect(0, 0, 100, 100);
  QLineF line(-5, -5, 105, 105);
  const bool ok = lineBoundedByRect(line, rect);
  BOOST_REQUIRE(ok);
  BOOST_CHECK_SMALL(line.p1().x(), 1e-10);
  BOOST_CHECK_SMALL(line.p1().y(), 1e-10);
  BOOST_CHECK_CLOSE(line.p2().x(), 100.0, 0.01);
  BOOST_CHECK_CLOSE(line.p2().y(), 100.0, 0.01);
}

BOOST_AUTO_TEST_CASE(test_line_outside_rect_no_intersection) {
  const QRectF rect(10, 10, 80, 80);
  QLineF line(5, 0, 5, 100);
  const bool ok = lineBoundedByRect(line, rect);
  BOOST_CHECK(!ok);
}

BOOST_AUTO_TEST_CASE(test_vertical_line_through_rect) {
  const QRectF rect(10, 10, 80, 80);
  QLineF line(50, 0, 50, 200);
  const bool ok = lineBoundedByRect(line, rect);
  BOOST_REQUIRE(ok);
  BOOST_CHECK_CLOSE(line.p1().x(), 50.0, 0.01);
  BOOST_CHECK_CLOSE(line.p1().y(), 10.0, 0.01);
  BOOST_CHECK_CLOSE(line.p2().x(), 50.0, 0.01);
  BOOST_CHECK_CLOSE(line.p2().y(), 90.0, 0.01);
}

BOOST_AUTO_TEST_SUITE_END()
