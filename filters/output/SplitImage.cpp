
#include "SplitImage.h"
#include <imageproc/ColorTable.h>
#include <imageproc/ImageCombination.h>
#include <imageproc/RasterOp.h>
#include <cassert>

using namespace imageproc;

namespace output {
SplitImage::SplitImage() = default;

SplitImage::SplitImage(const QImage& foreground, const QImage& background) {
  if ((foreground.format() != QImage::Format_Mono) && (foreground.format() != QImage::Format_MonoLSB)
      && (foreground.format() != QImage::Format_Indexed8) && (foreground.format() != QImage::Format_RGB32)
      && (foreground.format() != QImage::Format_ARGB32)) {
    return;
  }
  if ((background.format() != QImage::Format_Indexed8) && (background.format() != QImage::Format_RGB32)
      && (background.format() != QImage::Format_ARGB32)) {
    return;
  }

  if (foreground.size() != background.size()) {
    return;
  }

  foregroundImage = foreground;
  backgroundImage = background;
}

SplitImage::SplitImage(const QImage& foreground, const QImage& background, const QImage& originalBackground)
    : SplitImage(foreground, background) {
  if (isNull()) {
    return;
  }

  if (originalBackground.size() != background.size()) {
    return;
  }

  if (originalBackground.format() != background.format()) {
    return;
  }

  if ((originalBackground.format() != QImage::Format_Indexed8) && (originalBackground.format() != QImage::Format_RGB32)
      && (originalBackground.format() != QImage::Format_ARGB32)) {
    return;
  }

  originalBackgroundImage = originalBackground;
}

QImage SplitImage::toImage() const {
  if (isNull()) {
    return QImage();
  }

  if (originalBackgroundImage.isNull()) {
    if (!mask.isNull()) {
      return backgroundImage;
    }

    QImage dst(backgroundImage);
    combineImage(dst, foregroundImage);

    return dst;
  } else {
    QImage dst(originalBackgroundImage);

    {
      BinaryImage backgroundMask = BinaryImage(originalBackgroundImage, BinaryThreshold(1));
      combineImage(dst, backgroundImage, backgroundMask);
    }
    {
      BinaryImage foregroundMask = BinaryImage(originalBackgroundImage, BinaryThreshold(255)).inverted();
      combineImage(dst, (mask.isNull()) ? foregroundImage : backgroundImage, foregroundMask);
    }

    return dst;
  }
}

QImage SplitImage::getForegroundImage() const {
  if (!mask.isNull()) {
    QImage foreground(backgroundImage);
    applyMask(foreground, mask);

    if (binaryForeground) {
      foreground = foreground.convertToFormat(QImage::Format_Mono);
    } else if (indexedForeground) {
      foreground = ColorTable(foreground).toIndexedImage();
    }

    return foreground;
  }

  return foregroundImage;
}

void SplitImage::setForegroundImage(const QImage& foregroundImage) {
  mask = BinaryImage();
  SplitImage::foregroundImage = foregroundImage;
}

QImage SplitImage::getBackgroundImage() const {
  if (!mask.isNull()) {
    QImage background(backgroundImage);
    applyMask(background, mask.inverted());

    return background;
  }

  return backgroundImage;
}

void SplitImage::setBackgroundImage(const QImage& backgroundImage) {
  SplitImage::backgroundImage = backgroundImage;
}

void SplitImage::applyToLayerImages(const std::function<void(QImage&)>& consumer) {
  if (!foregroundImage.isNull()) {
    consumer(foregroundImage);
  }
  if (!backgroundImage.isNull()) {
    consumer(backgroundImage);
  }
  if (!originalBackgroundImage.isNull()) {
    consumer(originalBackgroundImage);
  }
}

bool SplitImage::isNull() const {
  return (foregroundImage.isNull() && mask.isNull()) || backgroundImage.isNull();
}

void SplitImage::setMask(const BinaryImage& mask, bool binaryForeground) {
  foregroundImage = QImage();
  SplitImage::mask = mask;
  SplitImage::binaryForeground = binaryForeground;
}

const QImage& SplitImage::getOriginalBackgroundImage() const {
  return originalBackgroundImage;
}

void SplitImage::setOriginalBackgroundImage(const QImage& originalBackgroundImage) {
  SplitImage::originalBackgroundImage = originalBackgroundImage;
}

void SplitImage::setIndexedForeground(bool indexedForeground) {
  SplitImage::indexedForeground = indexedForeground;
}
}  // namespace output
