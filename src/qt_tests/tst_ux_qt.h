// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef TST_UX_QT_H
#define TST_UX_QT_H

#include <QObject>

class TestUxQt : public QObject {
  Q_OBJECT
 private slots:
  void testButtonClickEmitsSignal();
  void testCheckBoxToggleChangesState();
  void testLineEditAcceptsInput();
  void testCollapsibleGroupBoxStateAndSignal();
  void testErrorWidgetCreation();
};

#endif
