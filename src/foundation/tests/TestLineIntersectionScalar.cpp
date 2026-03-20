// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include <LineIntersectionScalar.h>

#include <QLineF>
#include <QPointF>
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(FoundationLineIntersectionScalarTestSuite)

BOOST_AUTO_TEST_CASE(test_intersecting_lines) {
  const QLineF line1(0, 0, 10, 0);
  const QLineF line2(5, -5, 5, 5);
  double s1 = 0, s2 = 0;
  const bool ok = lineIntersectionScalar(line1, line2, s1, s2);
  BOOST_REQUIRE(ok);
  BOOST_CHECK_CLOSE(s1, 0.5, 0.01);
  BOOST_CHECK_CLOSE(s2, 0.5, 0.01);
  const QPointF pt1 = line1.pointAt(s1);
  const QPointF pt2 = line2.pointAt(s2);
  BOOST_CHECK_CLOSE(pt1.x(), 5.0, 0.01);
  BOOST_CHECK_CLOSE(pt1.y(), 0.0, 0.01);
  BOOST_CHECK_CLOSE(pt2.x(), 5.0, 0.01);
  BOOST_CHECK_CLOSE(pt2.y(), 0.0, 0.01);
}

BOOST_AUTO_TEST_CASE(test_parallel_lines) {
  const QLineF line1(0, 0, 10, 0);
  const QLineF line2(0, 1, 10, 1);
  double s1 = 0, s2 = 0;
  const bool ok = lineIntersectionScalar(line1, line2, s1, s2);
  BOOST_CHECK(!ok);
}

BOOST_AUTO_TEST_CASE(test_overlapping_lines) {
  const QLineF line1(0, 0, 10, 0);
  const QLineF line2(2, 0, 8, 0);
  double s1 = 0, s2 = 0;
  const bool ok = lineIntersectionScalar(line1, line2, s1, s2);
  BOOST_CHECK(!ok);
}

BOOST_AUTO_TEST_CASE(test_single_param_overload) {
  const QLineF line1(0, 0, 10, 0);
  const QLineF line2(3, 1, 3, -1);
  double s1 = 0;
  const bool ok = lineIntersectionScalar(line1, line2, s1);
  BOOST_REQUIRE(ok);
  BOOST_CHECK_CLOSE(s1, 0.3, 0.01);
  BOOST_CHECK_CLOSE(line1.pointAt(s1).x(), 3.0, 0.01);
}

BOOST_AUTO_TEST_SUITE_END()
