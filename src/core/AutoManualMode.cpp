// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "AutoManualMode.h"

QString autoManualModeToString(AutoManualMode mode) {
  QString str;
  switch (mode) {
    case MODE_AUTO:
      str = "auto";
      break;
    case MODE_MANUAL:
      str = "manual";
      break;
    case MODE_DISABLED:
      str = "disabled";
      break;
  }
  return str;
}

AutoManualMode stringToAutoManualMode(const QString& str) {
  if (str == "disabled") {
    return MODE_DISABLED;
  } else if (str == "manual") {
    return MODE_MANUAL;
  } else {
    return MODE_AUTO;
  }
}