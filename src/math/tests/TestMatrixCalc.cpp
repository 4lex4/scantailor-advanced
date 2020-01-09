// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include <MatrixCalc.h>
#include <boost/test/unit_test.hpp>
#include <boost/test/tools/floating_point_comparison.hpp>

namespace imageproc {
namespace tests {
BOOST_AUTO_TEST_SUITE(MatrixCalcSuite)

BOOST_AUTO_TEST_CASE(test1) {
  static const double A[] = {1, 1, 1, 2, 4, -3, 3, 6, -5};

  static const double B[] = {9, 1, 0};

  static const double control[] = {7, -1, 3};

  double x[3];

  MatrixCalc<double> mc;
  mc(A, 3, 3).trans().solve(mc(B, 3, 1)).write(x);

  for (int i = 0; i < 3; ++i) {
    BOOST_REQUIRE_CLOSE(x[i], control[i], 1e-6);
  }
}

BOOST_AUTO_TEST_CASE(test2) {
  static const double A[] = {1, 1, 1, 2, 4, -3, 3, 6, -5, 3, 5, -2, 5, 10, -8};

  double B[] = {9, 1, 0, 10, 1};

  static const double control[] = {7, -1, 3};

  double x[3];

  MatrixCalc<double> mc;
  mc(A, 3, 5).trans().solve(mc(B, 5, 1)).write(x);

  for (int i = 0; i < 3; ++i) {
    BOOST_REQUIRE_CLOSE(x[i], control[i], 1e-6);
  }

  // Now make the system inconsistent.
  B[4] += 1.0;
  BOOST_CHECK_THROW(mc(A, 3, 5).trans().solve(mc(B, 5, 1)), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(test3) {
  static const double A[] = {1, 3, 1, 1, 1, 2, 2, 3, 4};

  static const double control[] = {2, 9, -5, 0, -2, 1, -1, -3, 2};

  double inv[9];

  MatrixCalc<double> mc;
  mc(A, 3, 3).trans().inv().transWrite(inv);

  for (int i = 0; i < 9; ++i) {
    BOOST_REQUIRE_CLOSE(inv[i], control[i], 1e-6);
  }
}

BOOST_AUTO_TEST_CASE(test4) {
  static const double A[] = {4, 1, 9, 6, 2, 8, 7, 3, 5, 11, 10, 12};

  static const double B[] = {2, 9, 5, 12, 8, 10};

  static const double control[] = {85, 138, 86, 158, 69, 149, 168, 339};

  double mul[8];

  MatrixCalc<double> mc;
  (mc(A, 3, 4).trans() * (mc(B, 2, 3).trans())).transWrite(mul);

  for (int i = 0; i < 8; ++i) {
    BOOST_REQUIRE_CLOSE(mul[i], control[i], 1e-6);
  }
}

BOOST_AUTO_TEST_SUITE_END()
}  // namespace tests
}  // namespace imageproc