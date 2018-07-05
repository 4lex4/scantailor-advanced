#ifndef SCANTAILOR_ADVANCED_EMPTYTASKSTATUS_H
#define SCANTAILOR_ADVANCED_EMPTYTASKSTATUS_H

#include "TaskStatus.h"

class EmptyTaskStatus : public TaskStatus {
  void cancel() override {}

  bool isCancelled() const override { return false; }

  void throwIfCancelled() const override {}
};

#endif  // SCANTAILOR_ADVANCED_EMPTYTASKSTATUS_H
