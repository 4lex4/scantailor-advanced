
#ifndef SCANTAILOR_UNITSLISTENER_H
#define SCANTAILOR_UNITSLISTENER_H


#include "Units.h"

class Dpi;

class UnitsListener {
 protected:
  UnitsListener();

 public:
  virtual ~UnitsListener();

  virtual void onUnitsChanged(Units units) = 0;
};

#endif  //SCANTAILOR_UNITSLISTENER_H
