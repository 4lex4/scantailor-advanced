// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_CORE_OUTOFMEMORYHANDLER_H_
#define SCANTAILOR_CORE_OUTOFMEMORYHANDLER_H_

#include <QMutex>
#include <QObject>
#include <boost/scoped_array.hpp>
#include <cstddef>
#include "NonCopyable.h"

class OutOfMemoryHandler : public QObject {
  Q_OBJECT
  DECLARE_NON_COPYABLE(OutOfMemoryHandler)

 public:
  static OutOfMemoryHandler& instance();

  /**
   * To be called once, before any OOM situations can occur.
   */
  void allocateEmergencyMemory(size_t bytes);

  /** May be called from any thread. */
  void handleOutOfMemorySituation();

  bool hadOutOfMemorySituation() const;

 signals:

  /** Will be dispatched from the main thread. */
  void outOfMemory();

 private:
  OutOfMemoryHandler();

  mutable QMutex m_mutex;
  boost::scoped_array<char> m_emergencyBuffer;
  bool m_hadOOM;
};


#endif  // ifndef SCANTAILOR_CORE_OUTOFMEMORYHANDLER_H_
