// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "QtSignalForwarder.h"

QtSignalForwarder::QtSignalForwarder(QObject* emitter, const char* signal, const boost::function<void()>& slot)
    : QObject(emitter), m_slot(slot) {
  connect(emitter, signal, SLOT(handleSignal()));
}

void QtSignalForwarder::handleSignal() {
  m_slot();
}
