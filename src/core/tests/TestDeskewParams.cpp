// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include <Dpi.h>
#include <ImageTransformation.h>
#include <filters/deskew/Dependencies.h>
#include <filters/deskew/Params.h>

#include <QDomDocument>
#include <QPolygonF>
#include <QRectF>
#include <QTransform>
#include <boost/test/unit_test.hpp>

namespace Tests {
using namespace deskew;

BOOST_AUTO_TEST_SUITE(DeskewParamsTestSuite)

BOOST_AUTO_TEST_CASE(params_oblique_roundtrip_xml) {
  const double deskewDeg = 1.5;
  const double obliqueDeg = 2.25;
  const Dependencies deps;
  const Params original(deskewDeg, obliqueDeg, deps, MODE_MANUAL);

  QDomDocument doc;
  const QDomElement el = original.toXml(doc, "deskew-params");
  doc.appendChild(el);

  const Params restored(doc.documentElement());

  BOOST_CHECK_CLOSE(restored.deskewAngle(), deskewDeg, 1e-6);
  BOOST_CHECK_CLOSE(restored.obliqueAngle(), obliqueDeg, 1e-6);
  BOOST_CHECK(restored.mode() == MODE_MANUAL);
}

BOOST_AUTO_TEST_CASE(params_zero_oblique_roundtrip_xml) {
  const double deskewDeg = -0.5;
  const Dependencies deps;
  const Params original(deskewDeg, 0.0, deps, MODE_AUTO);

  QDomDocument doc;
  const QDomElement el = original.toXml(doc, "deskew-params");
  doc.appendChild(el);

  const Params restored(doc.documentElement());

  BOOST_CHECK_CLOSE(restored.deskewAngle(), deskewDeg, 1e-6);
  BOOST_CHECK_CLOSE(restored.obliqueAngle(), 0.0, 1e-6);
  BOOST_CHECK(restored.mode() == MODE_AUTO);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(ImageTransformationObliqueTestSuite)

BOOST_AUTO_TEST_CASE(post_oblique_applied_and_stored) {
  const QRectF rect(0, 0, 100, 100);
  const Dpi dpi(300, 300);
  ImageTransformation xform(rect, dpi);

  QPolygonF preCrop;
  preCrop << rect.topLeft() << rect.topRight() << rect.bottomRight() << rect.bottomLeft();
  xform.setPreCropArea(preCrop);

  xform.setPostOblique(0.0);
  const QTransform transformZeroOblique = xform.transform();
  const double obliqueDeg = 2.0;
  xform.setPostOblique(obliqueDeg);

  BOOST_CHECK_CLOSE(xform.postOblique(), obliqueDeg, 1e-6);
  BOOST_CHECK(transformZeroOblique != xform.transform());
}

BOOST_AUTO_TEST_SUITE_END()
}  // namespace Tests
