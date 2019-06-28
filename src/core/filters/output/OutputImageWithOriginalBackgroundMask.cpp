#include "OutputImageWithOriginalBackgroundMask.h"
#include <imageproc/Dpm.h>
#include <imageproc/ImageCombination.h>

using namespace imageproc;

namespace output {
OutputImageWithOriginalBackgroundMask::OutputImageWithOriginalBackgroundMask(const QImage& image,
                                                                             const BinaryImage& foregroundMask,
                                                                             ForegroundType foregroundType,
                                                                             const BinaryImage& backgroundMask)
    : OutputImageWithForegroundMask(image, foregroundMask, foregroundType), m_backgroundMask(backgroundMask) {}

bool OutputImageWithOriginalBackgroundMask::isNull() const {
  return OutputImageWithForegroundMask::isNull() || m_backgroundMask.isNull();
}

QImage OutputImageWithOriginalBackgroundMask::getBackgroundImage() const {
  QImage background = OutputImageWithForegroundMask::getBackgroundImage();
  applyMask(background, m_backgroundMask);
  return background;
}

QImage OutputImageWithOriginalBackgroundMask::getOriginalBackgroundImage() const {
  QImage originalBackground = OutputImageWithForegroundMask::getBackgroundImage();
  applyMask(originalBackground, m_backgroundMask.inverted(), BLACK);
  return originalBackground;
}

std::unique_ptr<OutputImageWithOriginalBackgroundMask> OutputImageWithOriginalBackgroundMask::fromPlainData(
    const QImage& foregroundImage,
    const QImage& backgroundImage,
    const QImage& originalBackground) {
  if (foregroundImage.isNull() || backgroundImage.isNull() || originalBackground.isNull()) {
    return std::make_unique<OutputImageWithOriginalBackgroundMask>();
  }

  BinaryImage foregroundMask = BinaryImage(originalBackground, BinaryThreshold(255)).inverted();
  BinaryImage backgroundMask = BinaryImage(originalBackground, BinaryThreshold(1));

  QImage image(originalBackground);
  combineImages(image, foregroundImage, foregroundMask);
  combineImages(image, backgroundImage, backgroundMask);

  return std::make_unique<OutputImageWithOriginalBackgroundMask>(image, foregroundMask,
                                                                 getForegroundType(foregroundImage), backgroundMask);
}
}  // namespace output