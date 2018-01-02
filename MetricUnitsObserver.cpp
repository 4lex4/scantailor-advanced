
#include "MetricUnitsObserver.h"
#include "MetricUnitsProvider.h"
#include <QObject>

MetricUnitsObserver::MetricUnitsObserver() {
    MetricUnitsProvider::getInstance()->attachObserver(this);
}

MetricUnitsObserver::~MetricUnitsObserver() {
    MetricUnitsProvider::getInstance()->detachObserver(this);
}

void MetricUnitsObserver::updateDpi(Dpi dpi) {
}

QString toString(const MetricUnits units) {
    QString unitsStr;
    switch (units) {
        case PIXELS:
            unitsStr = QObject::tr("px");
            break;
        case MILLIMETRES:
            unitsStr = QObject::tr("mm");
            break;
        case CENTIMETRES:
            unitsStr = QObject::tr("cm");
            break;
        case INCHES:
            unitsStr = QObject::tr("in");
            break;
    }

    return unitsStr;
}