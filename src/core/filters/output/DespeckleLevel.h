// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_OUTPUT_DESPECKLELEVEL_H_
#define SCANTAILOR_OUTPUT_DESPECKLELEVEL_H_

class QString;

namespace output {
enum DespeckleLevel { DESPECKLE_OFF, DESPECKLE_CAUTIOUS, DESPECKLE_NORMAL, DESPECKLE_AGGRESSIVE };

QString despeckleLevelToString(DespeckleLevel level);

DespeckleLevel despeckleLevelFromString(const QString& str);
}  // namespace output
#endif
