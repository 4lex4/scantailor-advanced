// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include <ImageId.h>
#include <PageId.h>
#include <PageRange.h>

#include <boost/test/unit_test.hpp>
#include <set>

BOOST_AUTO_TEST_SUITE(CorePageRangeTestSuite)

BOOST_AUTO_TEST_CASE(test_select_every_other_empty_range) {
  PageRange range;
  const PageId base(ImageId("/x", 0), PageId::SINGLE_PAGE);
  const std::set<PageId> result = range.selectEveryOther(base);
  BOOST_CHECK(result.empty());
}

BOOST_AUTO_TEST_CASE(test_select_every_other_base_not_in_range) {
  PageRange range;
  range.pages.push_back(PageId(ImageId("/a", 0), PageId::SINGLE_PAGE));
  range.pages.push_back(PageId(ImageId("/b", 0), PageId::SINGLE_PAGE));
  const PageId base(ImageId("/c", 0), PageId::SINGLE_PAGE);
  const std::set<PageId> result = range.selectEveryOther(base);
  BOOST_CHECK(result.empty());
}

BOOST_AUTO_TEST_CASE(test_select_every_other_alternates) {
  PageRange range;
  const PageId p0(ImageId("/a", 0), PageId::SINGLE_PAGE);
  const PageId p1(ImageId("/b", 0), PageId::SINGLE_PAGE);
  const PageId p2(ImageId("/c", 0), PageId::SINGLE_PAGE);
  const PageId p3(ImageId("/d", 0), PageId::SINGLE_PAGE);
  range.pages = {p0, p1, p2, p3};

  std::set<PageId> result = range.selectEveryOther(p0);
  BOOST_CHECK_EQUAL(result.size(), 2u);
  BOOST_CHECK(result.count(p0) == 1);
  BOOST_CHECK(result.count(p2) == 1);
  BOOST_CHECK(result.count(p1) == 0);
  BOOST_CHECK(result.count(p3) == 0);

  result = range.selectEveryOther(p1);
  BOOST_CHECK_EQUAL(result.size(), 2u);
  BOOST_CHECK(result.count(p1) == 1);
  BOOST_CHECK(result.count(p3) == 1);
}

BOOST_AUTO_TEST_CASE(test_select_every_other_single_page) {
  PageRange range;
  const PageId p(ImageId("/x", 0), PageId::SINGLE_PAGE);
  range.pages.push_back(p);
  const std::set<PageId> result = range.selectEveryOther(p);
  BOOST_CHECK_EQUAL(result.size(), 1u);
  BOOST_CHECK(result.count(p) == 1);
}

BOOST_AUTO_TEST_SUITE_END()
