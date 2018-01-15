
#include <algorithm>
#include <QtCore/QSettings>
#include "MetricUnitsProvider.h"
#include "Dpm.h"

std::unique_ptr<MetricUnitsProvider> MetricUnitsProvider::instance = nullptr;

MetricUnitsProvider::MetricUnitsProvider()
        : metricUnits(metricUnitsFromString(QSettings().value("settings/metric_units", "mm").toString())) {
}

MetricUnitsProvider* MetricUnitsProvider::getInstance() {
    if (instance == nullptr) {
        instance.reset(new MetricUnitsProvider());
    }

    return instance.get();
}

const Dpi& MetricUnitsProvider::getDpi() const {
    return metricUnitsConverter.getDpi();
}

void MetricUnitsProvider::setDpi(const Dpi& dpi) {
    metricUnitsConverter.setDpi(dpi);
    dpiChanged();
}

MetricUnits MetricUnitsProvider::getMetricUnits() const {
    return metricUnits;
}

void MetricUnitsProvider::setMetricUnits(MetricUnits metricUnits) {
    MetricUnitsProvider::metricUnits = metricUnits;
    metricUnitsChanged();
}

void MetricUnitsProvider::attachObserver(MetricUnitsObserver* observer) {
    observers.push_back(observer);
}

void MetricUnitsProvider::detachObserver(MetricUnitsObserver* observer) {
    auto it = std::find(observers.begin(), observers.end(), observer);
    if (it != observers.end()) {
        observers.erase(it);
    }
}

void MetricUnitsProvider::dpiChanged() {
    for (MetricUnitsObserver* observer : observers) {
        observer->updateDpi(metricUnitsConverter.getDpi());
    }
}

void MetricUnitsProvider::metricUnitsChanged() {
    for (MetricUnitsObserver* observer : observers) {
        observer->updateMetricUnits(metricUnits);
    }
}

void MetricUnitsProvider::convert(double& horizontalValue,
                                  double& verticalValue,
                                  MetricUnits fromUnits,
                                  MetricUnits toUnits) const {
    metricUnitsConverter.convert(horizontalValue, verticalValue, fromUnits, toUnits);
}

void MetricUnitsProvider::convertFrom(double& horizontalValue, double& verticalValue, MetricUnits fromUnits) const {
    convert(horizontalValue, verticalValue, fromUnits, metricUnits);
}

void MetricUnitsProvider::convertTo(double& horizontalValue, double& verticalValue, MetricUnits toUnits) const {
    convert(horizontalValue, verticalValue, metricUnits, toUnits);
}
