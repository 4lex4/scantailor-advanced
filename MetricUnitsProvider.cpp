
#include <algorithm>
#include "MetricUnitsProvider.h"
#include "Dpm.h"

std::unique_ptr<MetricUnitsProvider> MetricUnitsProvider::instance = nullptr;

MetricUnitsProvider::MetricUnitsProvider()
        : metricUnits(MILLIMETRES) {
}

MetricUnitsProvider* MetricUnitsProvider::getInstance() {
    if (instance == nullptr) {
        instance.reset(new MetricUnitsProvider());
    }

    return instance.get();
}

const Dpi& MetricUnitsProvider::getDpi() const {
    return dpi;
}

void MetricUnitsProvider::setDpi(const Dpi& dpi) {
    MetricUnitsProvider::dpi = dpi;
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
        observer->updateDpi(dpi);
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
    if (dpi.isNull() || (fromUnits == toUnits)) {
        return;
    }

    Dpm dpm = Dpm(dpi);
    switch (fromUnits) {
        case PIXELS:
            switch (toUnits) {
                case MILLIMETRES:
                    horizontalValue = horizontalValue / dpm.horizontal() * 1000.;
                    verticalValue = verticalValue / dpm.vertical() * 1000.;
                    break;
                case CENTIMETRES:
                    horizontalValue = horizontalValue / dpm.horizontal() * 100.;
                    verticalValue = verticalValue / dpm.vertical() * 100.;
                    break;
                case INCHES:
                    horizontalValue /= dpi.horizontal();
                    verticalValue /= dpi.vertical();
                    break;
                default:
                    break;
            }
            break;
        case MILLIMETRES:
            switch (toUnits) {
                case PIXELS:
                    horizontalValue = horizontalValue / 1000. * dpm.horizontal();
                    verticalValue = verticalValue / 1000. * dpm.vertical();
                    break;
                case CENTIMETRES:
                    horizontalValue = horizontalValue / 10.;
                    verticalValue = verticalValue / 10.;
                    break;
                case INCHES:
                    horizontalValue = horizontalValue / 1000. * dpm.horizontal() / dpi.horizontal();
                    verticalValue = verticalValue / 1000. * dpm.vertical() / dpi.vertical();
                    break;
                default:
                    break;
            }
            break;
        case CENTIMETRES:
            switch (toUnits) {
                case PIXELS:
                    horizontalValue = horizontalValue / 100. * dpm.horizontal();
                    verticalValue = verticalValue / 100. * dpm.vertical();
                    break;
                case MILLIMETRES:
                    horizontalValue = horizontalValue * 10.;
                    verticalValue = verticalValue * 10.;
                    break;
                case INCHES:
                    horizontalValue = horizontalValue / 100. * dpm.horizontal() / dpi.horizontal();
                    verticalValue = verticalValue / 100. * dpm.vertical() / dpi.vertical();
                    break;
                default:
                    break;
            }
            break;
        case INCHES:
            switch (toUnits) {
                case PIXELS:
                    horizontalValue *= dpi.horizontal();
                    verticalValue *= dpi.vertical();
                    break;
                case MILLIMETRES:
                    horizontalValue = horizontalValue * dpi.horizontal() / dpm.horizontal() * 1000.;
                    verticalValue = verticalValue * dpi.vertical() / dpm.vertical() * 1000.;
                    break;
                case CENTIMETRES:
                    horizontalValue = horizontalValue * dpi.horizontal() / dpm.horizontal() * 100.;
                    verticalValue = verticalValue * dpi.vertical() / dpm.vertical() * 100.;
                    break;
                default:
                    break;
            }
            break;
    }
}

void MetricUnitsProvider::convertFrom(double& horizontalValue, double& verticalValue, MetricUnits fromUnits) const {
    convert(horizontalValue, verticalValue, fromUnits, metricUnits);
}

void MetricUnitsProvider::convertTo(double& horizontalValue, double& verticalValue, MetricUnits toUnits) const {
    convert(horizontalValue, verticalValue, metricUnits, toUnits);
}
