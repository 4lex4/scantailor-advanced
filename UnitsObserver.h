
#ifndef SCANTAILOR_UNITSOBSERVER_H
#define SCANTAILOR_UNITSOBSERVER_H


#include "Units.h"

class Dpi;

class UnitsObserver {
 public:
  UnitsObserver();

  virtual ~UnitsObserver();

  virtual void updateUnits(Units units) = 0;
};

#endif  // SCANTAILOR_UNITSOBSERVER_H
