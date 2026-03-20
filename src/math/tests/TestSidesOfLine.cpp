// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include <SidesOfLine.h>

#include <QLineF>
#include <QPointF>
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(MathSidesOfLineTestSuite)

BOOST_AUTO_TEST_CASE(test_points_on_same_side_positive) {
  const QLineF line(0, 0, 10, 0);
  const QPointF p1(5, 1);
  const QPointF p2(5, 2);
  const qreal result = sidesOfLine(line, p1, p2);
  BOOST_CHECK(result > 0);
}

BOOST_AUTO_TEST_CASE(test_points_on_opposite_sides) {
  const QLineF line(0, 0, 10, 0);
  const QPointF p1(5, 1);
  const QPointF p2(5, -1);
  const qreal result = sidesOfLine(line, p1, p2);
  BOOST_CHECK(result < 0);
}

BOOST_AUTO_TEST_CASE(test_point_on_line_returns_zero) {
  const QLineF line(0, 0, 10, 0);
  const QPointF p1(5, 0);
  const QPointF p2(5, 1);
  const qreal result = sidesOfLine(line, p1, p2);
  BOOST_CHECK_SMALL(result, 1e-6);
}

BOOST_AUTO_TEST_CASE(test_both_points_on_line_returns_zero) {
  const QLineF line(0, 0, 10, 0);
  const QPointF p1(2, 0);
  const QPointF p2(7, 0);
  const qreal result = sidesOfLine(line, p1, p2);
  BOOST_CHECK_SMALL(result, 1e-6);
}

BOOST_AUTO_TEST_CASE(test_diagonal_line_same_side) {
  const QLineF line(0, 0, 10, 10);
  const QPointF p1(0, 1);
  const QPointF p2(1, 2);
  const qreal result = sidesOfLine(line, p1, p2);
  BOOST_CHECK(result > 0);
}

BOOST_AUTO_TEST_SUITE_END()
