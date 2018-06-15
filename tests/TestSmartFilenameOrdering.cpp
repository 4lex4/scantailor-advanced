/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2007-2008  Joseph Artsimovich <joseph_a@mail.ru>

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

#include <QFileInfo>
#include <QString>
#include <boost/test/auto_unit_test.hpp>
#include "SmartFilenameOrdering.h"

namespace Tests {
BOOST_AUTO_TEST_SUITE(SmartFilenameOrderingTestSuite);

BOOST_AUTO_TEST_CASE(test_same_file) {
  const SmartFilenameOrdering less;
  const QFileInfo somefile("/etc/somefile");
  BOOST_CHECK(!less(somefile, somefile));
}

BOOST_AUTO_TEST_CASE(test_dirs_different) {
  const SmartFilenameOrdering less;
  const QFileInfo lhs("/etc/file");
  const QFileInfo rhs("/ect/file");
  BOOST_CHECK(less(lhs, rhs) == (lhs.absolutePath() < rhs.absolutePath()));
  BOOST_CHECK(less(rhs, lhs) == (rhs.absolutePath() < lhs.absolutePath()));
}

BOOST_AUTO_TEST_CASE(test_simple_case) {
  const SmartFilenameOrdering less;
  const QFileInfo lhs("/etc/1.png");
  const QFileInfo rhs("/etc/2.png");
  BOOST_CHECK(less(lhs, rhs));
  BOOST_CHECK(!less(rhs, lhs));
}

BOOST_AUTO_TEST_CASE(test_avg_case) {
  const SmartFilenameOrdering less;
  const QFileInfo lhs("/etc/a_0002.png");
  const QFileInfo rhs("/etc/a_1.png");
  BOOST_CHECK(!less(lhs, rhs));
  BOOST_CHECK(less(rhs, lhs));
}

BOOST_AUTO_TEST_CASE(test_compex_case) {
  const SmartFilenameOrdering less;
  const QFileInfo lhs("/etc/a10_10.png");
  const QFileInfo rhs("/etc/a010_2.png");
  BOOST_CHECK(!less(lhs, rhs));
  BOOST_CHECK(less(rhs, lhs));
}

BOOST_AUTO_TEST_CASE(test_almost_equal) {
  const SmartFilenameOrdering less;
  const QFileInfo lhs("/etc/10.png");
  const QFileInfo rhs("/etc/010.png");
  BOOST_CHECK(!less(lhs, rhs));
  BOOST_CHECK(less(rhs, lhs));
}

BOOST_AUTO_TEST_SUITE_END();
}  // namespace Tests