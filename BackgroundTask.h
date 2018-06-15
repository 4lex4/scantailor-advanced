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

#ifndef BACKGROUNDTASK_H_
#define BACKGROUNDTASK_H_

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
    const char* what() const throw() override;
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

#endif  // ifndef BACKGROUNDTASK_H_
