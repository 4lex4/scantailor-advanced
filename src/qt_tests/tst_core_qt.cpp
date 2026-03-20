// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "tst_core_qt.h"

#include <SmartFilenameOrdering.h>
#include <Units.h>
#include <foundation/Utils.h>

#include <QFileInfo>
#include <QSignalSpy>
#include <QString>
#include <QtTest/QtTest>

void TestCoreQt::testUnitsToString() {
  QCOMPARE(unitsToString(PIXELS), QString("px"));
  QCOMPARE(unitsToString(MILLIMETRES), QString("mm"));
  QCOMPARE(unitsToString(CENTIMETRES), QString("cm"));
  QCOMPARE(unitsToString(INCHES), QString("in"));
}

void TestCoreQt::testUnitsFromString() {
  QCOMPARE(unitsFromString(QString("px")), PIXELS);
  QCOMPARE(unitsFromString(QString("mm")), MILLIMETRES);
  QCOMPARE(unitsFromString(QString("cm")), CENTIMETRES);
  QCOMPARE(unitsFromString(QString("in")), INCHES);
  QCOMPARE(unitsFromString(QString("unknown")), MILLIMETRES);
}

void TestCoreQt::testUnitsToLocalizedString() {
  QString px = unitsToLocalizedString(PIXELS);
  QString mm = unitsToLocalizedString(MILLIMETRES);
  QVERIFY(!px.isEmpty());
  QVERIFY(!mm.isEmpty());
}

void TestCoreQt::testUtilsDoubleToString() {
  QCOMPARE(foundation::Utils::doubleToString(0.0), QString("0"));
  QVERIFY(foundation::Utils::doubleToString(123.456).contains("123"));
  QVERIFY(foundation::Utils::doubleToString(-42.5).startsWith("-"));
}

void TestCoreQt::testSmartFilenameOrdering() {
  const SmartFilenameOrdering less;
  const QFileInfo a("/etc/1.png");
  const QFileInfo b("/etc/2.png");
  QVERIFY(less(a, b));
  QVERIFY(!less(b, a));
  QVERIFY(!less(a, a));

  const QFileInfo c("/etc/a_0002.png");
  const QFileInfo d("/etc/a_1.png");
  QVERIFY(less(d, c));
}

void TestCoreQt::testSignalSpy() {
  Emitter obj;
  QSignalSpy spy(&obj, &Emitter::valueChanged);
  QVERIFY(spy.isValid());
  QCOMPARE(spy.count(), 0);

  obj.doEmit(QString("hello"));
  QCOMPARE(spy.count(), 1);
  QCOMPARE(spy.at(0).at(0).toString(), QString("hello"));

  obj.doEmit(QString("world"));
  QCOMPARE(spy.count(), 2);
  QCOMPARE(spy.at(1).at(0).toString(), QString("world"));
}
