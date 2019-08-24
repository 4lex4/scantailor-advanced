// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include <BinaryImage.h>
#include <SkewFinder.h>
#include <QApplication>
#include <QColor>
#include <QImage>
#include <QPainter>
#include <QString>
#include <QTransform>
#include <boost/test/auto_unit_test.hpp>
#include <cmath>
#include <cstdlib>

namespace imageproc {
namespace tests {
BOOST_AUTO_TEST_SUITE(SkewFinderTestSuite)

BOOST_AUTO_TEST_CASE(test_positive_detection) {
  int argc = 1;
  char argv0[] = "test";
  char* argv[1] = {argv0};
  QApplication app(argc, argv);
  QImage image(1000, 800, QImage::Format_ARGB32_Premultiplied);
  image.fill(0xffffffff);
  {
    QPainter painter(&image);
    painter.setPen(QColor(0, 0, 0));
    QTransform xform1;
    xform1.translate(-0.5 * image.width(), -0.5 * image.height());
    QTransform xform2;
    xform2.rotate(4.5);
    QTransform xform3;
    xform3.translate(0.5 * image.width(), 0.5 * image.height());
    painter.setWorldTransform(xform1 * xform2 * xform3);

    QString text;
    for (int line = 0; line < 40; ++line) {
      for (int i = 0; i < 100; ++i) {
        text += '1';
      }
      text += '\n';
    }
    QTextOption opt;
    opt.setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    painter.drawText(image.rect(), text, opt);
  }

  SkewFinder skewFinder;
  const Skew skew(skewFinder.findSkew(BinaryImage(image)));
  BOOST_REQUIRE(std::fabs(skew.angle() - 4.5) < 0.15);
  BOOST_CHECK(skew.confidence() >= Skew::GOOD_CONFIDENCE);
}

BOOST_AUTO_TEST_CASE(test_negative_detection) {
  QImage image(1000, 800, QImage::Format_Mono);
  image.fill(1);

  const int numDots = image.width() * image.height() / 5;
  for (int i = 0; i < numDots; ++i) {
    const int x = rand() % image.width();
    const int y = rand() % image.height();
    image.setPixel(x, y, 0);
  }

  SkewFinder skewFinder;
  skewFinder.setCoarseReduction(0);
  skewFinder.setFineReduction(0);
  const Skew skew(skewFinder.findSkew(BinaryImage(image)));
  BOOST_CHECK(skew.confidence() < Skew::GOOD_CONFIDENCE);
}

BOOST_AUTO_TEST_SUITE_END()
}  // namespace tests
}  // namespace imageproc