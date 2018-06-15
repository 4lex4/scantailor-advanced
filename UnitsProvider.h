
#ifndef SCANTAILOR_UNITSPROVIDER_H
#define SCANTAILOR_UNITSPROVIDER_H

#include <list>
#include <memory>
#include "UnitsObserver.h"

class Dpi;

class UnitsProvider {
 private:
  static std::unique_ptr<UnitsProvider> instance;

  std::list<UnitsObserver*> observers;
  Units units;

  UnitsProvider();

 public:
  static UnitsProvider* getInstance();

  Units getUnits() const;

  void setUnits(Units units);

  void attachObserver(UnitsObserver* observer);

  void detachObserver(UnitsObserver* observer);

  void convertFrom(double& horizontalValue, double& verticalValue, Units fromUnits, const Dpi& dpi) const;

  void convertTo(double& horizontalValue, double& verticalValue, Units toUnits, const Dpi& dpi) const;

 protected:
  void unitsChanged();
};


#endif  // SCANTAILOR_UNITSPROVIDER_H
