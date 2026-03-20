// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include <ImageId.h>
#include <PageId.h>

#include <QString>
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(CorePageIdTestSuite)

BOOST_AUTO_TEST_CASE(test_default_constructor_is_null) {
  const PageId id;
  BOOST_CHECK(id.isNull());
}

BOOST_AUTO_TEST_CASE(test_subpage_to_string) {
  BOOST_CHECK(PageId::subPageToString(PageId::SINGLE_PAGE) == "single");
  BOOST_CHECK(PageId::subPageToString(PageId::LEFT_PAGE) == "left");
  BOOST_CHECK(PageId::subPageToString(PageId::RIGHT_PAGE) == "right");
}

BOOST_AUTO_TEST_CASE(test_subpage_from_string) {
  bool ok = false;
  BOOST_CHECK(PageId::subPageFromString("single", &ok) == PageId::SINGLE_PAGE);
  BOOST_CHECK(ok);
  BOOST_CHECK(PageId::subPageFromString("left", &ok) == PageId::LEFT_PAGE);
  BOOST_CHECK(ok);
  BOOST_CHECK(PageId::subPageFromString("right", &ok) == PageId::RIGHT_PAGE);
  BOOST_CHECK(ok);

  PageId::subPageFromString("invalid", &ok);
  BOOST_CHECK(!ok);
  BOOST_CHECK(PageId::subPageFromString("", &ok) == PageId::SINGLE_PAGE);
  BOOST_CHECK(!ok);
}

BOOST_AUTO_TEST_CASE(test_constructor_with_image_id) {
  const ImageId imageId("/path/to/image.tiff", 0);
  const PageId pageId(imageId, PageId::LEFT_PAGE);
  BOOST_CHECK(!pageId.isNull());
  BOOST_CHECK(pageId.imageId() == imageId);
  BOOST_CHECK(pageId.subPage() == PageId::LEFT_PAGE);
  BOOST_CHECK(pageId.subPageAsString() == "left");
}

BOOST_AUTO_TEST_CASE(test_equality) {
  const ImageId img1("/a", 0);
  const ImageId img2("/b", 0);
  const PageId p1(img1, PageId::SINGLE_PAGE);
  const PageId p2(img1, PageId::SINGLE_PAGE);
  const PageId p3(img2, PageId::SINGLE_PAGE);
  const PageId p4(img1, PageId::LEFT_PAGE);

  BOOST_CHECK(p1 == p2);
  BOOST_CHECK(p1 != p3);
  BOOST_CHECK(p1 != p4);
}

BOOST_AUTO_TEST_CASE(test_ordering) {
  const ImageId img1("/a", 0);
  const ImageId img2("/b", 0);
  const PageId p1(img1, PageId::SINGLE_PAGE);
  const PageId p2(img1, PageId::LEFT_PAGE);
  const PageId p3(img1, PageId::RIGHT_PAGE);
  const PageId p4(img2, PageId::SINGLE_PAGE);

  BOOST_CHECK(p1 < p2);
  BOOST_CHECK(p2 < p3);
  BOOST_CHECK(p3 < p4);
  BOOST_CHECK(!(p1 < p1));
}

BOOST_AUTO_TEST_SUITE_END()
