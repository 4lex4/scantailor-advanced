#ifndef SCANTAILOR_OUTPUTIMAGE_H
#define SCANTAILOR_OUTPUTIMAGE_H

#include <QImage>

class Dpm;

namespace output {
class OutputImage {
 public:
  virtual ~OutputImage() = default;

  virtual QImage toImage() const = 0;

  virtual bool isNull() const = 0;

  virtual void setDpm(const Dpm& dpm) = 0;

  virtual operator QImage() const { return toImage(); }
};
}  // namespace output


#endif  // SCANTAILOR_OUTPUTIMAGE_H
