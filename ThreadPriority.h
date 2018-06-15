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

#ifndef THREAD_PRIORITY_H_
#define THREAD_PRIORITY_H_

#include <QString>
#include <QThread>

class QSettings;

class ThreadPriority {
  // Member-wise copying is OK.
 public:
  enum Priority { Minimum, Idle = Minimum, Lowest, Low, Normal, Maximum = Normal };

  ThreadPriority(Priority prio) : m_prio(prio) {}

  void setValue(Priority prio) { m_prio = prio; }

  Priority value() const { return m_prio; }

  QThread::Priority toQThreadPriority() const;

  int toPosixNiceLevel() const;

  static ThreadPriority load(const QSettings& settings, const QString& key, Priority dflt = Normal);

  static ThreadPriority load(const QString& key, Priority dflt = Normal);

  void save(QSettings& settings, const QString& key);

  void save(const QString& key);

 private:
  Priority m_prio;
};


#endif  // ifndef THREAD_PRIORITY_H_
