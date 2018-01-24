
#include <QtCore/QObject>
#include "Units.h"

QString toString(const Units units) {
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

Units unitsFromString(const QString& string) {
    if (string == "px") {
        return PIXELS;
    } else if (string == "cm") {
        return CENTIMETRES;
    } else if (string == "in") {
        return INCHES;
    } else {
        return MILLIMETRES;
    }
}