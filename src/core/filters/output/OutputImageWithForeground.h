// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_OUTPUTIMAGEWITHFOREGROUND_H
#define SCANTAILOR_OUTPUTIMAGEWITHFOREGROUND_H

#include "OutputImage.h"

namespace output {
class OutputImageWithForeground : public virtual OutputImage {
 public:
  virtual QImage getForegroundImage() const = 0;

  virtual QImage getBackgroundImage() const = 0;
};
}

#endif //SCANTAILOR_OUTPUTIMAGEWITHFOREGROUND_H
