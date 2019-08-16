// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_OUTPUTIMAGEWITHORIGINALBACKGROUND_H
#define SCANTAILOR_OUTPUTIMAGEWITHORIGINALBACKGROUND_H

#include "OutputImageWithForeground.h"

namespace output {
class OutputImageWithOriginalBackground : public virtual OutputImageWithForeground {
 public:
  virtual QImage getOriginalBackgroundImage() const = 0;
};
}


#endif //SCANTAILOR_OUTPUTIMAGEWITHORIGINALBACKGROUND_H
