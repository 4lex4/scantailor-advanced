// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include <QuadraticFunction.h>

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(MathQuadraticFunctionTestSuite)

BOOST_AUTO_TEST_CASE(test_constructor) {
  QuadraticFunction f(2);
  BOOST_CHECK_EQUAL(f.numVars(), 2u);
  BOOST_CHECK_SMALL(f.c, 1e-10);
  BOOST_CHECK_EQUAL(f.b.size(), 2u);
  BOOST_CHECK_EQUAL(f.A.rows(), 2u);
  BOOST_CHECK_EQUAL(f.A.cols(), 2u);
}

BOOST_AUTO_TEST_CASE(test_evaluate_quadratic) {
  QuadraticFunction f(2);
  f.A(0, 0) = 1.0;
  f.A(1, 1) = 1.0;
  f.b[0] = 1.0;
  f.b[1] = 1.0;
  f.c = 0.0;
  double x[] = {2.0, 3.0};
  const double val = f.evaluate(x);
  BOOST_CHECK_CLOSE(val, 2 * 2 + 3 * 3 + 2 + 3, 1e-10);
  BOOST_CHECK_CLOSE(val, 4.0 + 9.0 + 5.0, 1e-10);
}

BOOST_AUTO_TEST_CASE(test_reset) {
  QuadraticFunction f(2);
  f.A(0, 0) = 1.0;
  f.b[0] = 1.0;
  f.c = 1.0;
  f.reset();
  BOOST_CHECK_SMALL(f.A(0, 0), 1e-10);
  BOOST_CHECK_SMALL(f.b[0], 1e-10);
  BOOST_CHECK_SMALL(f.c, 1e-10);
}

BOOST_AUTO_TEST_CASE(test_operator_plus_equals) {
  QuadraticFunction f1(2);
  f1.A(0, 0) = 1.0;
  f1.b[0] = 1.0;
  f1.c = 1.0;
  QuadraticFunction f2(2);
  f2.A(1, 1) = 1.0;
  f2.b[1] = 1.0;
  f2.c = 2.0;
  f1 += f2;
  BOOST_CHECK_CLOSE(f1.A(0, 0), 1.0, 1e-10);
  BOOST_CHECK_CLOSE(f1.A(1, 1), 1.0, 1e-10);
  BOOST_CHECK_CLOSE(f1.b[0], 1.0, 1e-10);
  BOOST_CHECK_CLOSE(f1.b[1], 1.0, 1e-10);
  BOOST_CHECK_CLOSE(f1.c, 3.0, 1e-10);
}

BOOST_AUTO_TEST_CASE(test_operator_times_equals_scalar) {
  QuadraticFunction f(2);
  f.A(0, 0) = 2.0;
  f.b[0] = 4.0;
  f.c = 6.0;
  f *= 0.5;
  BOOST_CHECK_CLOSE(f.A(0, 0), 1.0, 1e-10);
  BOOST_CHECK_CLOSE(f.b[0], 2.0, 1e-10);
  BOOST_CHECK_CLOSE(f.c, 3.0, 1e-10);
}

BOOST_AUTO_TEST_CASE(test_gradient) {
  QuadraticFunction f(2);
  f.A(0, 0) = 2.0;
  f.A(0, 1) = 1.0;
  f.A(1, 0) = 1.0;
  f.A(1, 1) = 2.0;
  f.b[0] = 1.0;
  f.b[1] = 2.0;
  f.c = 0.0;
  const auto grad = f.gradient();
  BOOST_CHECK_EQUAL(grad.A.rows(), 2u);
  BOOST_CHECK_EQUAL(grad.A.cols(), 2u);
  BOOST_CHECK_CLOSE(grad.b[0], 1.0, 1e-10);
  BOOST_CHECK_CLOSE(grad.b[1], 2.0, 1e-10);
}

BOOST_AUTO_TEST_CASE(test_recalc_for_translated_arguments) {
  QuadraticFunction f(2);
  f.A(0, 0) = 1.0;
  f.b[0] = 0.0;
  f.c = 0.0;
  double trans[] = {1.0, 0.0};
  f.recalcForTranslatedArguments(trans);
  BOOST_CHECK_CLOSE(f.b[0], 2.0, 1e-10);
  BOOST_CHECK_CLOSE(f.c, 1.0, 1e-10);
}

BOOST_AUTO_TEST_SUITE_END()
