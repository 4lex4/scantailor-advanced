// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include <QLineF>
#include <QPointF>
#include <boost/foreach.hpp>
#include <boost/test/auto_unit_test.hpp>
#include <set>
#include <vector>
#include <RastLineFinder.h>

namespace imageproc {
namespace tests {
BOOST_AUTO_TEST_SUITE(RastLineFinderTestSuite)

static bool matchSupportPoints(const std::vector<unsigned>& idxs1, const std::set<unsigned>& idxs2) {
  return std::set<unsigned>(idxs1.begin(), idxs1.end()) == idxs2;
}

BOOST_AUTO_TEST_CASE(test1) {
  // 4- and 3-point lines with min_support_points == 3
  // --------------------------------------------------
  // x     x
  // x x
  // x x
  // x
  // --------------------------------------------------
  std::vector<QPointF> pts;
  pts.emplace_back(-100, -100);
  pts.emplace_back(0, 0);
  pts.emplace_back(100, 100);
  pts.emplace_back(200, 200);
  pts.emplace_back(0, 100);
  pts.emplace_back(100, 0);
  pts.emplace_back(-100, 200);

  std::set<unsigned> line1_idxs;
  line1_idxs.insert(0);
  line1_idxs.insert(1);
  line1_idxs.insert(2);
  line1_idxs.insert(3);

  std::set<unsigned> line2_idxs;
  line2_idxs.insert(4);
  line2_idxs.insert(5);
  line2_idxs.insert(6);

  RastLineFinderParams params;
  params.setMinSupportPoints(3);
  RastLineFinder finder(pts, params);

  std::vector<unsigned> support_idxs;

  // line 1
  BOOST_REQUIRE(!finder.findNext(&support_idxs).isNull());
  BOOST_REQUIRE(matchSupportPoints(support_idxs, line1_idxs));

  // line2
  BOOST_REQUIRE(!finder.findNext(&support_idxs).isNull());
  BOOST_REQUIRE(matchSupportPoints(support_idxs, line2_idxs));

  // no more lines
  BOOST_REQUIRE(finder.findNext().isNull());
}

BOOST_AUTO_TEST_SUITE_END()
}  // namespace tests
}  // namespace imageproc
