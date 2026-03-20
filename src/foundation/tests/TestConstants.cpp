// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include <Constants.h>

#include <boost/test/unit_test.hpp>
#include <cmath>

BOOST_AUTO_TEST_SUITE(FoundationConstantsTestSuite)

BOOST_AUTO_TEST_CASE(test_pi) {
  BOOST_CHECK_CLOSE(constants::PI, 3.14159265358979323846, 0.0001);
}

BOOST_AUTO_TEST_CASE(test_sqrt_2) {
  BOOST_CHECK_CLOSE(constants::SQRT_2, std::sqrt(2.0), 0.0001);
}

BOOST_AUTO_TEST_CASE(test_deg2rad_rad2deg) {
  BOOST_CHECK_CLOSE(constants::DEG2RAD * 180.0, constants::PI, 0.0001);
  BOOST_CHECK_CLOSE(constants::RAD2DEG * constants::PI, 180.0, 0.0001);
}

BOOST_AUTO_TEST_CASE(test_inch_mm_conversion) {
  BOOST_CHECK_CLOSE(constants::INCH2MM, 25.4, 0.0001);
  BOOST_CHECK_CLOSE(constants::MM2INCH * 25.4, 1.0, 0.0001);
}

BOOST_AUTO_TEST_CASE(test_dpi_dpm_conversion) {
  BOOST_CHECK_CLOSE(constants::DPM2DPI, 0.0254, 0.0001);
  BOOST_CHECK_CLOSE(constants::DPI2DPM * constants::DPM2DPI, 1.0, 0.0001);
}

BOOST_AUTO_TEST_SUITE_END()
