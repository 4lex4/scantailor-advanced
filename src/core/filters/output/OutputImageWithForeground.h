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
