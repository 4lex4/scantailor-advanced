// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "BackgroundExecutor.h"

#include <QCoreApplication>
#include <QThread>
#include <cassert>

#include "OutOfMemoryHandler.h"

class BackgroundExecutor::Dispatcher : public QObject {
 public:
  explicit Dispatcher(Impl& owner);

 protected:
  void customEvent(QEvent* event) override;

 private:
  Impl& m_owner;
};


class BackgroundExecutor::Impl : public QThread {
 public:
  explicit Impl(BackgroundExecutor& owner);

  ~Impl() override;

  void enqueueTask(const TaskPtr& task);

 protected:
  void run() override;

  void customEvent(QEvent* event) override;

 private:
  BackgroundExecutor& m_owner;
  Dispatcher m_dispatcher;
  bool m_threadStarted;
};


/*============================ BackgroundExecutor ==========================*/

BackgroundExecutor::BackgroundExecutor() : m_impl(std::make_unique<Impl>(*this)) {}

BackgroundExecutor::~BackgroundExecutor() = default;

void BackgroundExecutor::shutdown() {
  m_impl.reset();
}

void BackgroundExecutor::enqueueTask(const TaskPtr& task) {
  if (m_impl) {
    m_impl->enqueueTask(task);
  }
}

/*===================== BackgroundExecutor::Dispatcher =====================*/

BackgroundExecutor::Dispatcher::Dispatcher(Impl& owner) : m_owner(owner) {}

void BackgroundExecutor::Dispatcher::customEvent(QEvent* event) {
  try {
    auto* evt = dynamic_cast<TaskEvent*>(event);
    assert(evt);

    const TaskPtr& task = evt->payload();
    assert(task);

    const TaskResultPtr result((*task)());
    if (result) {
      QCoreApplication::postEvent(&m_owner, new ResultEvent(result));
    }
  } catch (const std::bad_alloc&) {
    OutOfMemoryHandler::instance().handleOutOfMemorySituation();
  }
}

/*======================= BackgroundExecutor::Impl =========================*/

BackgroundExecutor::Impl::Impl(BackgroundExecutor& owner)
    : m_owner(owner), m_dispatcher(*this), m_threadStarted(false) {
  m_dispatcher.moveToThread(this);
}

BackgroundExecutor::Impl::~Impl() {
  exit();
  wait();
}

void BackgroundExecutor::Impl::enqueueTask(const TaskPtr& task) {
  QCoreApplication::postEvent(&m_dispatcher, new TaskEvent(task));
  if (!m_threadStarted) {
    start();
    m_threadStarted = true;
  }
}

void BackgroundExecutor::Impl::run() {
  exec();
}

void BackgroundExecutor::Impl::customEvent(QEvent* event) {
  auto* evt = dynamic_cast<ResultEvent*>(event);
  assert(evt);

  const TaskResultPtr& result = evt->payload();
  assert(result);

  (*result)();
}
