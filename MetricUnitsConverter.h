
#ifndef SCANTAILOR_METRICUNITSCONVERTER_H
#define SCANTAILOR_METRICUNITSCONVERTER_H


#include "Dpi.h"
#include "MetricUnitsObserver.h"

class MetricUnitsConverter {
private:
    Dpi dpi;

public:
    MetricUnitsConverter() = default;

    explicit MetricUnitsConverter(const Dpi& dpi);

    void convert(double& horizontalValue, double& verticalValue, MetricUnits fromUnits, MetricUnits toUnits) const;

    const Dpi& getDpi() const;

    void setDpi(const Dpi& dpi);
};


#endif //SCANTAILOR_METRICUNITSCONVERTER_H
