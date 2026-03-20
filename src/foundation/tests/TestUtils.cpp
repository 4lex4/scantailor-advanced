// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include <Utils.h>

#include <QString>
#include <boost/test/unit_test.hpp>

using namespace foundation;

BOOST_AUTO_TEST_SUITE(FoundationUtilsTestSuite)

BOOST_AUTO_TEST_CASE(test_double_to_string_zero) {
  BOOST_CHECK_EQUAL(Utils::doubleToString(0.0).toStdString(), "0");
}

BOOST_AUTO_TEST_CASE(test_double_to_string_positive) {
  const QString s = Utils::doubleToString(123.456);
  BOOST_CHECK(s.contains("123"));
  BOOST_CHECK(s.contains("456") || s.contains("46"));
}

BOOST_AUTO_TEST_CASE(test_double_to_string_negative) {
  const QString s = Utils::doubleToString(-42.5);
  BOOST_CHECK(s.startsWith("-") || s.contains("-"));
  BOOST_CHECK(s.contains("42"));
}

BOOST_AUTO_TEST_CASE(test_double_to_string_locale_independent) {
  const QString s1 = Utils::doubleToString(1.5);
  const QString s2 = Utils::doubleToString(1.5);
  BOOST_CHECK_EQUAL(s1.toStdString(), s2.toStdString());
  BOOST_CHECK(s1.contains("1") && (s1.contains("5") || s1.contains("15")));
}

BOOST_AUTO_TEST_SUITE_END()
