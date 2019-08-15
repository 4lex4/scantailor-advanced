#ifndef SCANTAILOR_ADVANCED_AUTOMANUALMODE_H
#define SCANTAILOR_ADVANCED_AUTOMANUALMODE_H

#include <QtCore/QString>

enum AutoManualMode { MODE_AUTO, MODE_MANUAL, MODE_DISABLED };

QString autoManualModeToString(AutoManualMode mode);

AutoManualMode stringToAutoManualMode(const QString& str);

#endif  // SCANTAILOR_ADVANCED_AUTOMANUALMODE_H
