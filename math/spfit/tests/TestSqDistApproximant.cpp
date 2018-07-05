/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C)  Joseph Artsimovich <joseph.artsimovich@gmail.com>

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

#include <QLineF>
#include <QPointF>
#include <boost/test/auto_unit_test.hpp>
#include <boost/test/floating_point_comparison.hpp>
#include <cmath>
#include <cstdlib>
#include "SqDistApproximant.h"
#include "ToLineProjector.h"

namespace spfit {
namespace tests {
BOOST_AUTO_TEST_SUITE(SqDistApproximantTestSuite);

static const double PI = 3.14159265;

static double frand(double from, double to) {
  const double rand_0_1 = rand() / double(RAND_MAX);

  return from + (to - from) * rand_0_1;
}

BOOST_AUTO_TEST_CASE(test_point_distance) {
  for (int i = 0; i < 100; ++i) {
    const Vec2d origin(frand(-50, 50), frand(-50, 50));
    const SqDistApproximant approx(SqDistApproximant::pointDistance(origin));
    for (int j = 0; j < 10; ++j) {
      const Vec2d pt(frand(-50, 50), frand(-50, 50));
      const double control = (pt - origin).squaredNorm();
      BOOST_REQUIRE_CLOSE(approx.evaluate(pt), control, 1e-06);
    }
  }
}

BOOST_AUTO_TEST_CASE(test_line_distance) {
  for (int i = 0; i < 100; ++i) {
    const Vec2d pt1(frand(-50, 50), frand(-50, 50));
    const double angle = frand(0, 2.0 * PI);
    const Vec2d delta(std::cos(angle), std::sin(angle));
    const QLineF line(pt1, pt1 + delta);
    const SqDistApproximant approx(SqDistApproximant::lineDistance(line));
    const ToLineProjector proj(line);
    for (int j = 0; j < 10; ++j) {
      const Vec2d pt(frand(-50, 50), frand(-50, 50));
      const double control = proj.projectionSqDist(pt);
      BOOST_REQUIRE_CLOSE(approx.evaluate(pt), control, 1e-06);
    }
  }
}

BOOST_AUTO_TEST_CASE(test_general_case) {
  for (int i = 0; i < 100; ++i) {
    const Vec2d origin(frand(-50, 50), frand(-50, 50));
    const double angle = frand(0, 2.0 * PI);
    const Vec2d u(std::cos(angle), std::sin(angle));
    Vec2d v(-u[1], u[0]);
    if (rand() & 1) {
      v = -v;
    }
    const double m = frand(0, 3);
    const double n = frand(0, 3);

    const SqDistApproximant approx(origin, u, v, m, n);

    for (int j = 0; j < 10; ++j) {
      const Vec2d pt(frand(-50, 50), frand(-50, 50));
      const double u_proj = u.dot(pt - origin);
      const double v_proj = v.dot(pt - origin);
      const double control = m * u_proj * u_proj + n * v_proj * v_proj;
      BOOST_REQUIRE_CLOSE(approx.evaluate(pt), control, 1e-06);
    }
  }
}

BOOST_AUTO_TEST_SUITE_END();
}  // namespace tests
}  // namespace spfit