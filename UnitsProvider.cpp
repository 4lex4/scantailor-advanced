
#include <algorithm>
#include <QtCore/QSettings>
#include "UnitsProvider.h"
#include "Dpm.h"

std::unique_ptr<UnitsProvider> UnitsProvider::instance = nullptr;

UnitsProvider::UnitsProvider()
        : units(unitsFromString(QSettings().value("settings/units", "mm").toString())) {
}

UnitsProvider* UnitsProvider::getInstance() {
    if (instance == nullptr) {
        instance.reset(new UnitsProvider());
    }

    return instance.get();
}

const Dpi& UnitsProvider::getDpi() const {
    return unitsConverter.getDpi();
}

void UnitsProvider::setDpi(const Dpi& dpi) {
    unitsConverter.setDpi(dpi);
    dpiChanged();
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
    auto it = std::find(observers.begin(), observers.end(), observer);
    if (it != observers.end()) {
        observers.erase(it);
    }
}

void UnitsProvider::dpiChanged() {
    for (UnitsObserver* observer : observers) {
        observer->updateDpi(unitsConverter.getDpi());
    }
}

void UnitsProvider::unitsChanged() {
    for (UnitsObserver* observer : observers) {
        observer->updateUnits(units);
    }
}

void UnitsProvider::convert(double& horizontalValue,
                                  double& verticalValue,
                                  Units fromUnits,
                                  Units toUnits) const {
    unitsConverter.convert(horizontalValue, verticalValue, fromUnits, toUnits);
}

void UnitsProvider::convertFrom(double& horizontalValue, double& verticalValue, Units fromUnits) const {
    convert(horizontalValue, verticalValue, fromUnits, units);
}

void UnitsProvider::convertTo(double& horizontalValue, double& verticalValue, Units toUnits) const {
    convert(horizontalValue, verticalValue, units, toUnits);
}
