// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef PERFORMANCETIMER_H_
#define PERFORMANCETIMER_H_

#include <ctime>

class PerformanceTimer {
 public:
  PerformanceTimer() : m_start(clock()) {}

  void print(const char* prefix = "");

 private:
  const clock_t m_start;
};


#endif
