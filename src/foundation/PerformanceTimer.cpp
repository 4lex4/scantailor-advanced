// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "PerformanceTimer.h"
#include <QDebug>

void PerformanceTimer::print(const char* prefix) {
  const clock_t now = clock();
  const double sec = double(now - m_start) / CLOCKS_PER_SEC;
  if (sec > 10.0) {
    qDebug() << prefix << (long) sec << " sec";
  } else if (sec > 0.01) {
    qDebug() << prefix << (long) (sec * 1000) << " msec";
  } else {
    qDebug() << prefix << (long) (sec * 1000000) << " usec";
  }
}
