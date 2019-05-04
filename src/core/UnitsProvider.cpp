
#include "UnitsProvider.h"
#include <QtCore/QSettings>
#include <algorithm>
#include "Dpm.h"
#include "UnitsConverter.h"

UnitsProvider::UnitsProvider() : m_units(unitsFromString(QSettings().value("settings/units", "mm").toString())) {}

UnitsProvider& UnitsProvider::getInstance() {
  static UnitsProvider instance;
  return instance;
}

Units UnitsProvider::getUnits() const {
  return m_units;
}

void UnitsProvider::setUnits(Units units) {
  UnitsProvider::m_units = units;
  unitsChanged();
}

void UnitsProvider::attachObserver(UnitsObserver* observer) {
  m_observers.push_back(observer);
}

void UnitsProvider::detachObserver(UnitsObserver* observer) {
  m_observers.remove(observer);
}

void UnitsProvider::unitsChanged() {
  for (UnitsObserver* observer : m_observers) {
    observer->updateUnits(m_units);
  }
}

void UnitsProvider::convertFrom(double& horizontalValue, double& verticalValue, Units fromUnits, const Dpi& dpi) const {
  UnitsConverter(dpi).convert(horizontalValue, verticalValue, fromUnits, m_units);
}

void UnitsProvider::convertTo(double& horizontalValue, double& verticalValue, Units toUnits, const Dpi& dpi) const {
  UnitsConverter(dpi).convert(horizontalValue, verticalValue, m_units, toUnits);
}
