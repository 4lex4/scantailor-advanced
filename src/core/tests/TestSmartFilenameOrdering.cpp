// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include <SmartFilenameOrdering.h>
#include <QFileInfo>
#include <QString>
#include <boost/test/auto_unit_test.hpp>

namespace Tests {
BOOST_AUTO_TEST_SUITE(SmartFilenameOrderingTestSuite)

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

BOOST_AUTO_TEST_SUITE_END()
}  // namespace Tests