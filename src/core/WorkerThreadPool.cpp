// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "WorkerThreadPool.h"

#include <QCoreApplication>
#include <QThreadPool>
#include <utility>

#include "OutOfMemoryHandler.h"

class WorkerThreadPool::TaskResultEvent : public QEvent {
 public:
  TaskResultEvent(BackgroundTaskPtr task, FilterResultPtr result)
      : QEvent(User), m_task(std::move(task)), m_result(std::move(result)) {}

  const BackgroundTaskPtr& task() const { return m_task; }

  const FilterResultPtr& result() const { return m_result; }

 private:
  BackgroundTaskPtr m_task;
  FilterResultPtr m_result;
};


WorkerThreadPool::WorkerThreadPool(QObject* parent) : QObject(parent), m_pool(new QThreadPool(this)) {
  updateNumberOfThreads();
}

WorkerThreadPool::~WorkerThreadPool() = default;

void WorkerThreadPool::shutdown() {
  m_pool->waitForDone();
}

bool WorkerThreadPool::hasSpareCapacity() const {
  return m_pool->activeThreadCount() < m_pool->maxThreadCount();
}

void WorkerThreadPool::submitTask(const BackgroundTaskPtr& task) {
  class Runnable : public QRunnable {
   public:
    Runnable(WorkerThreadPool& owner, BackgroundTaskPtr task) : m_owner(owner), m_task(std::move(task)) {
      setAutoDelete(true);
    }

    void run() override {
      if (m_task->isCancelled()) {
        return;
      }

      try {
        const FilterResultPtr result((*m_task)());
        if (result) {
          QCoreApplication::postEvent(&m_owner, new TaskResultEvent(m_task, result));
        }
      } catch (const std::bad_alloc&) {
        OutOfMemoryHandler::instance().handleOutOfMemorySituation();
      }
    }

   private:
    WorkerThreadPool& m_owner;
    BackgroundTaskPtr m_task;
  };


  updateNumberOfThreads();
  m_pool->start(new Runnable(*this, task));
}  // WorkerThreadPool::submitTask

void WorkerThreadPool::customEvent(QEvent* event) {
  if (auto* evt = dynamic_cast<TaskResultEvent*>(event)) {
    emit taskResult(evt->task(), evt->result());
  }
}

void WorkerThreadPool::updateNumberOfThreads() {
  int maxThreads = QThread::idealThreadCount();
  // Restricting num of processors for 32-bit due to
  // address space constraints.
  if (sizeof(void*) <= 4) {
    maxThreads = std::min(maxThreads, 2);
  }

  int numThreads = m_settings.value("settings/batch_processing_threads", maxThreads).toInt();
  numThreads = std::min(numThreads, maxThreads);
  m_pool->setMaxThreadCount(numThreads);
}
