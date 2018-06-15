
#include "UnitsProvider.h"
#include <QtCore/QSettings>
#include <algorithm>
#include "Dpm.h"
#include "UnitsConverter.h"

std::unique_ptr<UnitsProvider> UnitsProvider::instance = nullptr;

UnitsProvider::UnitsProvider() : units(unitsFromString(QSettings().value("settings/units", "mm").toString())) {}

UnitsProvider* UnitsProvider::getInstance() {
  if (instance == nullptr) {
    instance.reset(new UnitsProvider());
  }

  return instance.get();
}

Units UnitsProvider::getUnits() const {
  return units;
}

void UnitsProvider::setUnits(Units units) {
  UnitsProvider::units = units;
  unitsChanged();
}

void UnitsProvider::attachObserver(UnitsObserver* observer) {
  observers.push_back(observer);
}

void UnitsProvider::detachObserver(UnitsObserver* observer) {
  observers.remove(observer);
}

void UnitsProvider::unitsChanged() {
  for (UnitsObserver* observer : observers) {
    observer->updateUnits(units);
  }
}

void UnitsProvider::convertFrom(double& horizontalValue, double& verticalValue, Units fromUnits, const Dpi& dpi) const {
  UnitsConverter(dpi).convert(horizontalValue, verticalValue, fromUnits, units);
}

void UnitsProvider::convertTo(double& horizontalValue, double& verticalValue, Units toUnits, const Dpi& dpi) const {
  UnitsConverter(dpi).convert(horizontalValue, verticalValue, units, toUnits);
}
