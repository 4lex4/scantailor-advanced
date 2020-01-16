// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "OutOfMemoryHandler.h"

OutOfMemoryHandler::OutOfMemoryHandler() : m_hadOOM(false) {}

OutOfMemoryHandler& OutOfMemoryHandler::instance() {
  // Depending on the compiler, this may not be thread-safe.
  // However, because we insist an instance of this object to be created early on,
  // the only case that might get us into trouble is an out-of-memory situation
  // after main() has returned and this instance got destroyed.  This scenario
  // sounds rather fantastic, and is not a big deal, as the project would have
  // already been saved.
  static OutOfMemoryHandler object;
  return object;
}

void OutOfMemoryHandler::allocateEmergencyMemory(size_t bytes) {
  const QMutexLocker locker(&m_mutex);

  boost::scoped_array<char>(new char[bytes]).swap(m_emergencyBuffer);
}

void OutOfMemoryHandler::handleOutOfMemorySituation() {
  const QMutexLocker locker(&m_mutex);

  if (m_hadOOM) {
    return;
  }

  m_hadOOM = true;
  boost::scoped_array<char>().swap(m_emergencyBuffer);
  QMetaObject::invokeMethod(this, "outOfMemory", Qt::QueuedConnection);
}

bool OutOfMemoryHandler::hadOutOfMemorySituation() const {
  const QMutexLocker locker(&m_mutex);
  return m_hadOOM;
}
