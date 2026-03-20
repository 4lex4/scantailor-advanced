// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include <LinearFunction.h>

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(MathLinearFunctionTestSuite)

BOOST_AUTO_TEST_CASE(test_constructor_num_vars) {
  LinearFunction f(3);
  BOOST_CHECK_EQUAL(f.numVars(), 3u);
  BOOST_CHECK_EQUAL(f.a.size(), 3u);
  BOOST_CHECK_SMALL(f.b, 1e-10);
}

BOOST_AUTO_TEST_CASE(test_evaluate_linear) {
  LinearFunction f(2);
  f.a[0] = 1.0;
  f.a[1] = 2.0;
  f.b = 3.0;
  double x[] = {4.0, 5.0};
  const double val = f.evaluate(x);
  BOOST_CHECK_CLOSE(val, 1.0 * 4.0 + 2.0 * 5.0 + 3.0, 1e-10);
  BOOST_CHECK_CLOSE(val, 17.0, 1e-10);
}

BOOST_AUTO_TEST_CASE(test_reset) {
  LinearFunction f(2);
  f.a[0] = 1.0;
  f.a[1] = 1.0;
  f.b = 1.0;
  f.reset();
  BOOST_CHECK_SMALL(f.a[0], 1e-10);
  BOOST_CHECK_SMALL(f.a[1], 1e-10);
  BOOST_CHECK_SMALL(f.b, 1e-10);
}

BOOST_AUTO_TEST_CASE(test_operator_plus_equals) {
  LinearFunction f1(2);
  f1.a[0] = 1.0;
  f1.a[1] = 0.0;
  f1.b = 2.0;
  LinearFunction f2(2);
  f2.a[0] = 0.0;
  f2.a[1] = 3.0;
  f2.b = 1.0;
  f1 += f2;
  BOOST_CHECK_CLOSE(f1.a[0], 1.0, 1e-10);
  BOOST_CHECK_CLOSE(f1.a[1], 3.0, 1e-10);
  BOOST_CHECK_CLOSE(f1.b, 3.0, 1e-10);
}

BOOST_AUTO_TEST_CASE(test_operator_times_equals_scalar) {
  LinearFunction f(2);
  f.a[0] = 2.0;
  f.a[1] = 4.0;
  f.b = 6.0;
  f *= 0.5;
  BOOST_CHECK_CLOSE(f.a[0], 1.0, 1e-10);
  BOOST_CHECK_CLOSE(f.a[1], 2.0, 1e-10);
  BOOST_CHECK_CLOSE(f.b, 3.0, 1e-10);
}

BOOST_AUTO_TEST_CASE(test_swap) {
  LinearFunction f1(2);
  f1.a[0] = 1.0;
  f1.b = 2.0;
  LinearFunction f2(3);
  f2.a[0] = 5.0;
  f2.b = 6.0;
  swap(f1, f2);
  BOOST_CHECK_EQUAL(f1.numVars(), 3u);
  BOOST_CHECK_CLOSE(f1.b, 6.0, 1e-10);
  BOOST_CHECK_EQUAL(f2.numVars(), 2u);
  BOOST_CHECK_CLOSE(f2.b, 2.0, 1e-10);
}

BOOST_AUTO_TEST_SUITE_END()
