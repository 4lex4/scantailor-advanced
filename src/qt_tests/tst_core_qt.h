// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef TST_CORE_QT_H
#define TST_CORE_QT_H

#include <QObject>
#include <QString>

class TestCoreQt : public QObject {
  Q_OBJECT
 private slots:
  void testUnitsToString();
  void testUnitsFromString();
  void testUnitsToLocalizedString();
  void testUtilsDoubleToString();
  void testSmartFilenameOrdering();
  void testSignalSpy();
};

class Emitter : public QObject {
  Q_OBJECT
 public:
  explicit Emitter(QObject* parent = nullptr) : QObject(parent) {}
  void doEmit(const QString& value) { emit valueChanged(value); }
 signals:
  void valueChanged(const QString& value);
};

#endif
