// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_CORE_AUTOMANUALMODE_H_
#define SCANTAILOR_CORE_AUTOMANUALMODE_H_

#include <QtCore/QString>

enum AutoManualMode { MODE_AUTO, MODE_MANUAL, MODE_DISABLED };

QString autoManualModeToString(AutoManualMode mode);

AutoManualMode stringToAutoManualMode(const QString& str);

#endif  // SCANTAILOR_CORE_AUTOMANUALMODE_H_
