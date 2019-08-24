// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_CORE_BACKGROUNDEXECUTOR_H_
#define SCANTAILOR_CORE_BACKGROUNDEXECUTOR_H_

#include <memory>
#include "AbstractCommand.h"
#include "NonCopyable.h"
#include "PayloadEvent.h"
#include "intrusive_ptr.h"

class BackgroundExecutor {
  DECLARE_NON_COPYABLE(BackgroundExecutor)

 public:
  typedef intrusive_ptr<AbstractCommand<void>> TaskResultPtr;
  typedef intrusive_ptr<AbstractCommand<TaskResultPtr>> TaskPtr;

  BackgroundExecutor();

  /**
   * \brief Waits for background tasks to finish, then destroys the object.
   */
  ~BackgroundExecutor();

  /**
   * \brief Waits for pending jobs to finish and stop the background thread.
   *
   * The destructor also performs these tasks, so this method is only
   * useful to prematuraly stop task processing.  After shutdown, any
   * attempts to enqueue a task will be silently ignored.
   */
  void shutdown();

  /**
   * \brief Enqueue a task for execution in a background thread.
   *
   * A task is a functor to be executed in a background thread.
   * That functor may optionally return another one, that is
   * to be executed in the thread where this BackgroundExecutor
   * object was constructed.
   */
  void enqueueTask(const TaskPtr& task);

 private:
  class Impl;
  class Dispatcher;

  typedef PayloadEvent<TaskPtr> TaskEvent;
  typedef PayloadEvent<TaskResultPtr> ResultEvent;

  std::unique_ptr<Impl> m_impl;
};


#endif  // ifndef SCANTAILOR_CORE_BACKGROUNDEXECUTOR_H_
