// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_CORE_NULLTASKSTATUS_H_
#define SCANTAILOR_CORE_NULLTASKSTATUS_H_

#include "TaskStatus.h"

class NullTaskStatus : public TaskStatus {
  void cancel() override {}

  bool isCancelled() const override { return false; }

  void throwIfCancelled() const override {}
};

#endif  // SCANTAILOR_CORE_NULLTASKSTATUS_H_
