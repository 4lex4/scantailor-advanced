#ifndef SCANTAILOR_OUTPUTIMAGEWITHORIGINALBACKGROUNDMASK_H
#define SCANTAILOR_OUTPUTIMAGEWITHORIGINALBACKGROUNDMASK_H

#include <memory>
#include "OutputImageWithForegroundMask.h"
#include "OutputImageWithOriginalBackground.h"

namespace output {
class OutputImageWithOriginalBackgroundMask : public OutputImageWithForegroundMask,
                                              public virtual OutputImageWithOriginalBackground {
 public:
  static std::unique_ptr<OutputImageWithOriginalBackgroundMask> fromPlainData(const QImage& foregroundImage,
                                                                              const QImage& backgroundImage,
                                                                              const QImage& originalBackground);

  OutputImageWithOriginalBackgroundMask() = default;

  OutputImageWithOriginalBackgroundMask(const QImage& image,
                                        const imageproc::BinaryImage& foregroundMask,
                                        ForegroundType foregroundType,
                                        const imageproc::BinaryImage& backgroundMask);

  bool isNull() const override;

  QImage getBackgroundImage() const override;

  QImage getOriginalBackgroundImage() const override;

 private:
  imageproc::BinaryImage m_backgroundMask;
};
}  // namespace output

#endif  // SCANTAILOR_OUTPUTIMAGEWITHORIGINALBACKGROUNDMASK_H
