// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "tst_ux_qt.h"

#include <CollapsibleGroupBox.h>
#include <ErrorWidget.h>

#include <QCheckBox>
#include <QLineEdit>
#include <QPushButton>
#include <QSignalSpy>
#include <QWidget>
#include <QtTest/QtTest>

void TestUxQt::testButtonClickEmitsSignal() {
  QPushButton button("Click me");
  QSignalSpy spy(&button, &QPushButton::clicked);
  QVERIFY(spy.isValid());

  QTest::mouseClick(&button, Qt::LeftButton);
  QCOMPARE(spy.count(), 1);

  QTest::mouseClick(&button, Qt::LeftButton);
  QCOMPARE(spy.count(), 2);
}

void TestUxQt::testCheckBoxToggleChangesState() {
  QCheckBox box("Option");
  QVERIFY(!box.isChecked());

  box.setChecked(true);
  QVERIFY(box.isChecked());

  box.setChecked(false);
  QVERIFY(!box.isChecked());

  QSignalSpy spy(&box, &QCheckBox::toggled);
  box.setChecked(true);
  QCOMPARE(spy.count(), 1);
  QVERIFY(box.isChecked());
}

void TestUxQt::testLineEditAcceptsInput() {
  QLineEdit edit;
  QCOMPARE(edit.text(), QString());

  QTest::keyClicks(&edit, "Hello UX");
  QCOMPARE(edit.text(), QString("Hello UX"));

  edit.clear();
  QTest::keyClicks(&edit, "123");
  QCOMPARE(edit.text(), QString("123"));
}

void TestUxQt::testCollapsibleGroupBoxStateAndSignal() {
  CollapsibleGroupBox group("Options");
  QSignalSpy spy(&group, &CollapsibleGroupBox::collapsedStateChanged);
  QVERIFY(spy.isValid());

  QVERIFY(!group.isCollapsed());

  group.setCollapsed(true);
  QVERIFY(group.isCollapsed());
  QCOMPARE(spy.count(), 1);
  QCOMPARE(spy.at(0).at(0).toBool(), true);

  group.setCollapsed(false);
  QVERIFY(!group.isCollapsed());
  QCOMPARE(spy.count(), 2);
  QCOMPARE(spy.at(1).at(0).toBool(), false);
}

void TestUxQt::testErrorWidgetCreation() {
  ErrorWidget widget("Error de prueba", Qt::PlainText);
  QVERIFY(widget.findChildren<QWidget*>().size() >= 1);
}
