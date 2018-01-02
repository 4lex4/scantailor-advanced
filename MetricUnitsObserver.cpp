
#include "MetricUnitsObserver.h"
#include "MetricUnitsProvider.h"

MetricUnitsObserver::MetricUnitsObserver() {
    MetricUnitsProvider::getInstance()->attachObserver(this);
}

MetricUnitsObserver::~MetricUnitsObserver() {
    MetricUnitsProvider::getInstance()->detachObserver(this);
}

void MetricUnitsObserver::updateDpi(Dpi dpi) {
}
