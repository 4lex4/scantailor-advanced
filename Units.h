
#ifndef SCANTAILOR_UNITS_H
#define SCANTAILOR_UNITS_H


#include <QtCore/QString>

enum Units {
    PIXELS,
    MILLIMETRES,
    CENTIMETRES,
    INCHES
};

QString unitsToString(Units units);

Units unitsFromString(const QString& string);

#endif //SCANTAILOR_UNITS_H
