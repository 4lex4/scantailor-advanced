/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2015  Joseph Artsimovich <joseph.artsimovich@gmail.com>

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

#include "WorkerThreadPool.h"
#include "OutOfMemoryHandler.h"
#include <QCoreApplication>
#include <QThreadPool>

class WorkerThreadPool::TaskResultEvent : public QEvent {
public:
    TaskResultEvent(BackgroundTaskPtr const& task, FilterResultPtr const& result)
            : QEvent(User),
              m_ptrTask(task),
              m_ptrResult(result) {
    }

    BackgroundTaskPtr const& task() const {
        return m_ptrTask;
    }

    FilterResultPtr const& result() const {
        return m_ptrResult;
    }

private:
    BackgroundTaskPtr m_ptrTask;
    FilterResultPtr m_ptrResult;
};


WorkerThreadPool::WorkerThreadPool(QObject* parent)
        : QObject(parent),
          m_pPool(new QThreadPool(this)) {
    updateNumberOfThreads();
}

WorkerThreadPool::~WorkerThreadPool() {
}

void WorkerThreadPool::shutdown() {
    m_pPool->waitForDone();
}

bool WorkerThreadPool::hasSpareCapacity() const {
    return m_pPool->activeThreadCount() < m_pPool->maxThreadCount();
}

void WorkerThreadPool::submitTask(BackgroundTaskPtr const& task) {
    class Runnable : public QRunnable {
    public:
        Runnable(WorkerThreadPool& owner, BackgroundTaskPtr const& task)
                : m_rOwner(owner),
                  m_ptrTask(task) {
            setAutoDelete(true);
        }

        virtual void run()

        override {
            if (m_ptrTask->isCancelled()) {
                return;
            }

            try {
                FilterResultPtr const result((*m_ptrTask)());
                if (result) {
                    QCoreApplication::postEvent(
                            &m_rOwner, new TaskResultEvent(m_ptrTask, result)
                    );
                }
            } catch (std::bad_alloc const&) {
                OutOfMemoryHandler::instance().handleOutOfMemorySituation();
            }
        }

    private:
        WorkerThreadPool& m_rOwner;
        BackgroundTaskPtr m_ptrTask;
    };


    updateNumberOfThreads();
    m_pPool->start(new Runnable(*this, task));
}  // WorkerThreadPool::submitTask

void WorkerThreadPool::customEvent(QEvent* event) {
    if (TaskResultEvent* evt = dynamic_cast<TaskResultEvent*>(event)) {
        emit taskResult(evt->task(), evt->result());
    }
}

void WorkerThreadPool::updateNumberOfThreads() {
    int max_threads;
    if (sizeof(void*) <= 4) {
        // Restricting num of processors for 32-bit due to
        // address space constraints.
        max_threads = QThread::idealThreadCount();
        if (max_threads > 2) {
            max_threads = 2;
        }
    } else {
        max_threads = QThread::idealThreadCount();
    }

    int num_threads = m_settings.value("settings/batch_processing_threads", max_threads).toInt();
    num_threads = std::min<int>(num_threads, max_threads);
    m_pPool->setMaxThreadCount(num_threads);
}

