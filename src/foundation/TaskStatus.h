// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_FOUNDATION_TASKSTATUS_H_
#define SCANTAILOR_FOUNDATION_TASKSTATUS_H_

class TaskStatus {
 public:
  virtual ~TaskStatus() = default;

  virtual void cancel() = 0;

  virtual bool isCancelled() const = 0;

  virtual void throwIfCancelled() const = 0;
};


#endif
