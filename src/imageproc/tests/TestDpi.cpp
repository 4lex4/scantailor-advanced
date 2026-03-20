// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include <Dpi.h>
#include <Dpm.h>

#include <QDomDocument>
#include <QDomElement>
#include <QSize>
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(ImageProcDpiTestSuite)

BOOST_AUTO_TEST_CASE(test_default_constructor_is_null) {
  const Dpi dpi;
  BOOST_CHECK(dpi.isNull());
  BOOST_CHECK(dpi.horizontal() <= 1 || dpi.vertical() <= 1);
}

BOOST_AUTO_TEST_CASE(test_value_constructor) {
  const Dpi dpi(300, 600);
  BOOST_CHECK(!dpi.isNull());
  BOOST_CHECK_EQUAL(dpi.horizontal(), 300);
  BOOST_CHECK_EQUAL(dpi.vertical(), 600);
}

BOOST_AUTO_TEST_CASE(test_equality) {
  const Dpi a(300, 300);
  const Dpi b(300, 300);
  const Dpi c(300, 600);
  BOOST_CHECK(a == b);
  BOOST_CHECK(a != c);
}

BOOST_AUTO_TEST_CASE(test_to_size) {
  const Dpi dpi(72, 96);
  const QSize size = dpi.toSize();
  BOOST_CHECK_EQUAL(size.width(), 72);
  BOOST_CHECK_EQUAL(size.height(), 96);
}

BOOST_AUTO_TEST_CASE(test_to_size_null_returns_empty) {
  const Dpi dpi(0, 0);
  const QSize size = dpi.toSize();
  BOOST_CHECK(size.isEmpty());
}

BOOST_AUTO_TEST_CASE(test_from_qsize) {
  const QSize size(150, 200);
  const Dpi dpi(size);
  BOOST_CHECK_EQUAL(dpi.horizontal(), 150);
  BOOST_CHECK_EQUAL(dpi.vertical(), 200);
}

BOOST_AUTO_TEST_CASE(test_xml_roundtrip) {
  const Dpi original(300, 600);
  QDomDocument doc;
  QDomElement el = original.toXml(doc, "dpi");
  doc.appendChild(el);

  const QDomElement readEl = doc.documentElement();
  const Dpi fromXml(readEl);
  BOOST_CHECK_EQUAL(fromXml.horizontal(), original.horizontal());
  BOOST_CHECK_EQUAL(fromXml.vertical(), original.vertical());
}

BOOST_AUTO_TEST_CASE(test_from_xml_attributes) {
  QDomDocument doc;
  QDomElement el = doc.createElement("dpi");
  el.setAttribute("horizontal", "200");
  el.setAttribute("vertical", "400");
  const Dpi dpi(el);
  BOOST_CHECK_EQUAL(dpi.horizontal(), 200);
  BOOST_CHECK_EQUAL(dpi.vertical(), 400);
}

BOOST_AUTO_TEST_CASE(test_from_dpm) {
  const Dpm dpm(11811, 23622);
  const Dpi dpi(dpm);
  BOOST_CHECK(dpi.horizontal() > 0);
  BOOST_CHECK(dpi.vertical() > 0);
  BOOST_CHECK_CLOSE(static_cast<double>(dpi.horizontal()) / dpm.horizontal(), 0.0254, 0.1);
}

BOOST_AUTO_TEST_SUITE_END()
