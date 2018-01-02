
#ifndef SCANTAILOR_METRICUNITSOBSERVER_H
#define SCANTAILOR_METRICUNITSOBSERVER_H

#include "Dpi.h"

class Dpi;

enum MetricUnits {
    PIXELS,
    MILLIMETRES,
    CENTIMETRES,
    INCHES
};

class MetricUnitsObserver {
public:
    MetricUnitsObserver();

    virtual ~MetricUnitsObserver();

    virtual void updateDpi(Dpi dpi);

    virtual void updateMetricUnits(MetricUnits metricUnits) = 0;
};

#endif //SCANTAILOR_METRICUNITSOBSERVER_H
