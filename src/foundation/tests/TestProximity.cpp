// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include <Proximity.h>

#include <QLineF>
#include <QPointF>
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(FoundationProximityTestSuite)

BOOST_AUTO_TEST_CASE(test_default_constructor) {
  const Proximity p;
  BOOST_CHECK(p.dist() > 1e10);
  BOOST_CHECK(p.sqDist() > 1e20);
}

BOOST_AUTO_TEST_CASE(test_two_points) {
  const QPointF a(0, 0);
  const QPointF b(3, 4);
  const Proximity p(a, b);
  BOOST_CHECK_CLOSE(p.dist(), 5.0, 0.01);
  BOOST_CHECK_CLOSE(p.sqDist(), 25.0, 0.01);
}

BOOST_AUTO_TEST_CASE(test_same_point) {
  const QPointF a(1, 2);
  const Proximity p(a, a);
  BOOST_CHECK_SMALL(p.dist(), 1e-10);
  BOOST_CHECK_SMALL(p.sqDist(), 1e-20);
}

BOOST_AUTO_TEST_CASE(test_from_dist) {
  const Proximity p = Proximity::fromDist(10.0);
  BOOST_CHECK_CLOSE(p.dist(), 10.0, 0.01);
  BOOST_CHECK_CLOSE(p.sqDist(), 100.0, 0.01);
}

BOOST_AUTO_TEST_CASE(test_from_sq_dist) {
  const Proximity p = Proximity::fromSqDist(49.0);
  BOOST_CHECK_CLOSE(p.sqDist(), 49.0, 0.01);
  BOOST_CHECK_CLOSE(p.dist(), 7.0, 0.01);
}

BOOST_AUTO_TEST_CASE(test_comparison_operators) {
  const Proximity a = Proximity::fromSqDist(1.0);
  const Proximity b = Proximity::fromSqDist(4.0);
  BOOST_CHECK(a < b);
  BOOST_CHECK(a <= b);
  BOOST_CHECK(b > a);
  BOOST_CHECK(b >= a);
  BOOST_CHECK(a != b);
  BOOST_CHECK(!(a == b));
  BOOST_CHECK(Proximity::fromSqDist(2.0) == Proximity::fromSqDist(2.0));
}

BOOST_AUTO_TEST_CASE(test_point_and_line_segment_perpendicular) {
  const QPointF pt(5, 0);
  const QLineF segment(0, 0, 10, 0);
  QPointF pointOnSegment;
  const Proximity p = Proximity::pointAndLineSegment(pt, segment, &pointOnSegment);
  BOOST_CHECK_CLOSE(p.dist(), 0.0, 0.01);
  BOOST_CHECK_CLOSE(pointOnSegment.x(), 5.0, 0.01);
  BOOST_CHECK_CLOSE(pointOnSegment.y(), 0.0, 0.01);
}

BOOST_AUTO_TEST_CASE(test_point_and_line_segment_endpoint_closest) {
  const QPointF pt(-1, 0);
  const QLineF segment(0, 0, 10, 0);
  QPointF pointOnSegment;
  const Proximity prx = Proximity::pointAndLineSegment(pt, segment, &pointOnSegment);
  BOOST_CHECK_CLOSE(prx.dist(), 1.0, 0.01);
  BOOST_CHECK_CLOSE(pointOnSegment.x(), 0.0, 0.01);
  BOOST_CHECK_CLOSE(pointOnSegment.y(), 0.0, 0.01);
}

BOOST_AUTO_TEST_CASE(test_point_and_zero_length_segment) {
  const QPointF pt(1, 1);
  const QLineF segment(0, 0, 0, 0);
  const Proximity p = Proximity::pointAndLineSegment(pt, segment);
  BOOST_CHECK_CLOSE(p.dist(), std::sqrt(2.0), 0.01);
}

BOOST_AUTO_TEST_SUITE_END()
