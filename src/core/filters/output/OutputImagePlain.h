#ifndef SCANTAILOR_OUTPUTIMAGEPLAIN_H
#define SCANTAILOR_OUTPUTIMAGEPLAIN_H

#include "OutputImage.h"

namespace output {
class OutputImagePlain : public virtual OutputImage {
 public:
  OutputImagePlain() = default;

  explicit OutputImagePlain(const QImage& image);

  QImage toImage() const override;

  bool isNull() const override;

  void setDpm(const Dpm& dpm) override;

 private:
  QImage m_image;
};
}  // namespace output


#endif  //SCANTAILOR_OUTPUTIMAGEPLAIN_H
