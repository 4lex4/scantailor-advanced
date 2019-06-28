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
