// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include <Units.h>

#include <QString>
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(CoreUnitsTestSuite)

BOOST_AUTO_TEST_CASE(test_units_to_string) {
  BOOST_CHECK(unitsToString(PIXELS) == "px");
  BOOST_CHECK(unitsToString(MILLIMETRES) == "mm");
  BOOST_CHECK(unitsToString(CENTIMETRES) == "cm");
  BOOST_CHECK(unitsToString(INCHES) == "in");
}

BOOST_AUTO_TEST_CASE(test_units_from_string) {
  BOOST_CHECK(unitsFromString("px") == PIXELS);
  BOOST_CHECK(unitsFromString("mm") == MILLIMETRES);
  BOOST_CHECK(unitsFromString("cm") == CENTIMETRES);
  BOOST_CHECK(unitsFromString("in") == INCHES);
}

BOOST_AUTO_TEST_CASE(test_units_from_string_defaults_to_mm) {
  BOOST_CHECK(unitsFromString("") == MILLIMETRES);
  BOOST_CHECK(unitsFromString("unknown") == MILLIMETRES);
}

BOOST_AUTO_TEST_CASE(test_units_to_localized_string) {
  QString px = unitsToLocalizedString(PIXELS);
  QString mm = unitsToLocalizedString(MILLIMETRES);
  QString cm = unitsToLocalizedString(CENTIMETRES);
  QString in = unitsToLocalizedString(INCHES);
  BOOST_CHECK(!px.isEmpty());
  BOOST_CHECK(!mm.isEmpty());
  BOOST_CHECK(!cm.isEmpty());
  BOOST_CHECK(!in.isEmpty());
}

BOOST_AUTO_TEST_SUITE_END()
