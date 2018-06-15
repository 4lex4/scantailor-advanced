/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C)  Joseph Artsimovich <joseph.artsimovich@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

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
  Impl& m_rOwner;
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
  BackgroundExecutor& m_rOwner;
  Dispatcher m_dispatcher;
  bool m_threadStarted;
};


/*============================ BackgroundExecutor ==========================*/

BackgroundExecutor::BackgroundExecutor() : m_ptrImpl(new Impl(*this)) {}

BackgroundExecutor::~BackgroundExecutor() = default;

void BackgroundExecutor::shutdown() {
  m_ptrImpl.reset();
}

void BackgroundExecutor::enqueueTask(const TaskPtr& task) {
  if (m_ptrImpl) {
    m_ptrImpl->enqueueTask(task);
  }
}

/*===================== BackgroundExecutor::Dispatcher =====================*/

BackgroundExecutor::Dispatcher::Dispatcher(Impl& owner) : m_rOwner(owner) {}

void BackgroundExecutor::Dispatcher::customEvent(QEvent* event) {
  try {
    auto* evt = dynamic_cast<TaskEvent*>(event);
    assert(evt);

    const TaskPtr& task = evt->payload();
    assert(task);

    const TaskResultPtr result((*task)());
    if (result) {
      QCoreApplication::postEvent(&m_rOwner, new ResultEvent(result));
    }
  } catch (const std::bad_alloc&) {
    OutOfMemoryHandler::instance().handleOutOfMemorySituation();
  }
}

/*======================= BackgroundExecutor::Impl =========================*/

BackgroundExecutor::Impl::Impl(BackgroundExecutor& owner)
    : m_rOwner(owner), m_dispatcher(*this), m_threadStarted(false) {
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
