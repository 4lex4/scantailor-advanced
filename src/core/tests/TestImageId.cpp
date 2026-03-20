// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include <ImageId.h>

#include <QFileInfo>
#include <QString>
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(CoreImageIdTestSuite)

BOOST_AUTO_TEST_CASE(test_default_constructor_is_null) {
  const ImageId id;
  BOOST_CHECK(id.isNull());
  BOOST_CHECK(id.filePath().isNull());
  BOOST_CHECK(id.page() == 0);
}

BOOST_AUTO_TEST_CASE(test_constructor_with_path) {
  const ImageId id("/path/to/file.png", 0);
  BOOST_CHECK(!id.isNull());
  BOOST_CHECK(id.filePath() == "/path/to/file.png");
  BOOST_CHECK(id.page() == 0);
  BOOST_CHECK(!id.isMultiPageFile());
  BOOST_CHECK(id.zeroBasedPage() == 0);
}

BOOST_AUTO_TEST_CASE(test_multipage) {
  const ImageId id("/file.tiff", 3);
  BOOST_CHECK(id.page() == 3);
  BOOST_CHECK(id.isMultiPageFile());
  BOOST_CHECK(id.zeroBasedPage() == 2);
}

BOOST_AUTO_TEST_CASE(test_equality) {
  const ImageId a("/x", 1);
  const ImageId b("/x", 1);
  const ImageId c("/y", 1);
  const ImageId d("/x", 2);
  BOOST_CHECK(a == b);
  BOOST_CHECK(a != c);
  BOOST_CHECK(a != d);
}

BOOST_AUTO_TEST_CASE(test_ordering) {
  const ImageId a("/a", 0);
  const ImageId b("/b", 0);
  const ImageId c("/a", 1);
  BOOST_CHECK(a < b);
  BOOST_CHECK(a < c);
  BOOST_CHECK(c < b);
}

BOOST_AUTO_TEST_CASE(test_from_qfileinfo) {
  const QFileInfo info("/absolute/path/image.jpg");
  const ImageId id(info, 0);
  BOOST_CHECK(id.filePath() == "/absolute/path/image.jpg");
}

BOOST_AUTO_TEST_CASE(test_setters) {
  ImageId id;
  id.setFilePath("/new/path");
  id.setPage(2);
  BOOST_CHECK(id.filePath() == "/new/path");
  BOOST_CHECK(id.page() == 2);
}

BOOST_AUTO_TEST_SUITE_END()
