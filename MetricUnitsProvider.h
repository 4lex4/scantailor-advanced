
#ifndef SCANTAILOR_METRICUNITSPROVIDER_H
#define SCANTAILOR_METRICUNITSPROVIDER_H

#include <memory>
#include <list>
#include "MetricUnitsObserver.h"
#include "MetricUnitsConverter.h"

class Dpi;

class MetricUnitsProvider {
private:
    static std::unique_ptr<MetricUnitsProvider> instance;

    std::list<MetricUnitsObserver*> observers;
    MetricUnits metricUnits;
    MetricUnitsConverter metricUnitsConverter;

    MetricUnitsProvider();

public:
    static MetricUnitsProvider* getInstance();

    const Dpi& getDpi() const;

    void setDpi(const Dpi& dpi);

    MetricUnits getMetricUnits() const;

    void setMetricUnits(MetricUnits metricUnits);

    void attachObserver(MetricUnitsObserver* observer);

    void detachObserver(MetricUnitsObserver* observer);

    void convert(double& horizontalValue, double& verticalValue, MetricUnits fromUnits, MetricUnits toUnits) const;

    void convertFrom(double& horizontalValue, double& verticalValue, MetricUnits fromUnits) const;

    void convertTo(double& horizontalValue, double& verticalValue, MetricUnits toUnits) const;

protected:
    void dpiChanged();

    void metricUnitsChanged();
};


#endif //SCANTAILOR_METRICUNITSPROVIDER_H
