
#ifndef SCANTAILOR_UNITSPROVIDER_H
#define SCANTAILOR_UNITSPROVIDER_H

#include <memory>
#include <list>
#include "UnitsObserver.h"
#include "UnitsConverter.h"

class Dpi;

class UnitsProvider {
private:
    static std::unique_ptr<UnitsProvider> instance;

    std::list<UnitsObserver*> observers;
    Units units;
    UnitsConverter unitsConverter;

    UnitsProvider();

public:
    static UnitsProvider* getInstance();

    const Dpi& getDpi() const;

    void setDpi(const Dpi& dpi);

    Units getUnits() const;

    void setUnits(Units units);

    void attachObserver(UnitsObserver* observer);

    void detachObserver(UnitsObserver* observer);

    void convert(double& horizontalValue, double& verticalValue, Units fromUnits, Units toUnits) const;

    void convertFrom(double& horizontalValue, double& verticalValue, Units fromUnits) const;

    void convertTo(double& horizontalValue, double& verticalValue, Units toUnits) const;

protected:
    void dpiChanged();

    void unitsChanged();
};


#endif //SCANTAILOR_UNITSPROVIDER_H
