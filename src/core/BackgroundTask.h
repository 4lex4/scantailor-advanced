// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_CORE_BACKGROUNDTASK_H_
#define SCANTAILOR_CORE_BACKGROUNDTASK_H_

#include <QAtomicInt>
#include <exception>
#include "AbstractCommand.h"
#include "FilterResult.h"
#include "TaskStatus.h"
#include "intrusive_ptr.h"

class BackgroundTask : public AbstractCommand<FilterResultPtr>, public TaskStatus {
 public:
  enum Type { INTERACTIVE, BATCH };

  class CancelledException : public std::exception {
   public:
    const char* what() const noexcept override;
  };


  explicit BackgroundTask(Type type) : m_type(type) {}

  Type type() const { return m_type; }

  void cancel() override { m_cancelFlag.store(1); }

  bool isCancelled() const override { return m_cancelFlag.load() != 0; }

  /**
   * \brief If cancelled, throws CancelledException.
   */
  void throwIfCancelled() const override;

 private:
  QAtomicInt m_cancelFlag;
  const Type m_type;
};


typedef intrusive_ptr<BackgroundTask> BackgroundTaskPtr;

#endif  // ifndef SCANTAILOR_CORE_BACKGROUNDTASK_H_
