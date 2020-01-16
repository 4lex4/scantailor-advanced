// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include <adiff/Function.h>

#include <boost/test/tools/floating_point_comparison.hpp>
#include <boost/test/unit_test.hpp>
#include <cmath>
#include <cstdlib>

namespace adiff {
namespace tests {
BOOST_AUTO_TEST_SUITE(AutomaticDifferentiationTestSuite)

BOOST_AUTO_TEST_CASE(test1) {
  // F(x) = x^2  | x = 3

  SparseMap<2> sparseMap(1);
  sparseMap.markAllNonZero();

  const Function<2> x(0, 3, sparseMap);
  const Function<2> res(x * x);

  const VecT<double> gradient(res.gradient(sparseMap));
  const MatT<double> hessian(res.hessian(sparseMap));

  // F = 9
  // Fx = 2 * x = 6
  // Fxx = 2

  BOOST_REQUIRE_CLOSE(res.value, 9, 1e-06);
  BOOST_REQUIRE_CLOSE(gradient[0], 6, 1e-06);
  BOOST_REQUIRE_CLOSE(hessian(0, 0), 2, 1e-06);
}

BOOST_AUTO_TEST_CASE(test2) {
  // F(x, y) = x^2  | x = 3

  SparseMap<2> sparseMap(2);
  sparseMap.markAllNonZero();

  const Function<2> x(0, 3, sparseMap);
  const Function<2> res(x * x);

  const VecT<double> gradient(res.gradient(sparseMap));
  const MatT<double> hessian(res.hessian(sparseMap));

  // F = 9
  // Fx = 2 * x = 6
  // Fy = 0
  // Fxx = 2
  // Fyy = 0
  // Fxy = 0

  BOOST_REQUIRE_CLOSE(res.value, 9, 1e-06);
  BOOST_REQUIRE_CLOSE(gradient[0], 6, 1e-06);
  BOOST_REQUIRE_CLOSE(gradient[1], 0, 1e-06);
  BOOST_REQUIRE_CLOSE(hessian(0, 0), 2, 1e-06);
  BOOST_REQUIRE_CLOSE(hessian(1, 0), 0, 1e-06);
  BOOST_REQUIRE_CLOSE(hessian(0, 1), 0, 1e-06);
  BOOST_REQUIRE_CLOSE(hessian(1, 1), 0, 1e-06);
}

BOOST_AUTO_TEST_CASE(test3) {
  // F(x, y) = x^3 * y^2   | x = 2, y = 3

  SparseMap<2> sparseMap(2);
  sparseMap.markAllNonZero();

  const Function<2> x(0, 2, sparseMap);
  const Function<2> y(1, 3, sparseMap);
  const Function<2> xxx(x * x * x);
  const Function<2> yy(y * y);
  const Function<2> res(xxx * yy);

  const VecT<double> gradient(res.gradient(sparseMap));
  const MatT<double> hessian(res.hessian(sparseMap));

  // F = 72
  // Fx = 3 * x^2 * y^2 = 108
  // Fy = 2 * y * x^3 = 48
  // Fxx = 6 * x * y^2 = 108
  // Fyy = 2 * x^3 = 16
  // Fyx = 6 * y * x^2 = 72

  BOOST_REQUIRE_CLOSE(res.value, 72, 1e-06);
  BOOST_REQUIRE_CLOSE(gradient[0], 108, 1e-06);
  BOOST_REQUIRE_CLOSE(gradient[1], 48, 1e-06);
  BOOST_REQUIRE_CLOSE(hessian(0, 0), 108, 1e-06);
  BOOST_REQUIRE_CLOSE(hessian(0, 1), 72, 1e-06);
  BOOST_REQUIRE_CLOSE(hessian(1, 0), 72, 1e-06);
  BOOST_REQUIRE_CLOSE(hessian(1, 1), 16, 1e-06);
}

BOOST_AUTO_TEST_SUITE_END()
}  // namespace tests
}  // namespace adiff