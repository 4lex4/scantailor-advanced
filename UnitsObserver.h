
#ifndef SCANTAILOR_UNITSOBSERVER_H
#define SCANTAILOR_UNITSOBSERVER_H

#include "Dpi.h"
#include <QtCore/QString>

class Dpi;

enum Units {
    PIXELS,
    MILLIMETRES,
    CENTIMETRES,
    INCHES
};

QString toString(Units units);

Units unitsFromString(const QString& string);

class UnitsObserver {
public:
    UnitsObserver();

    virtual ~UnitsObserver();

    virtual void updateDpi(Dpi dpi);

    virtual void updateUnits(Units units) = 0;
};

#endif //SCANTAILOR_UNITSOBSERVER_H
