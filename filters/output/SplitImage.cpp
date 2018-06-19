
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

  m_foregroundImage = foreground;
  m_backgroundImage = background;
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

  m_originalBackgroundImage = originalBackground;
}

QImage SplitImage::toImage() const {
  if (isNull()) {
    return QImage();
  }

  if (m_originalBackgroundImage.isNull()) {
    if (!m_mask.isNull()) {
      return m_backgroundImage;
    }

    QImage dst(m_backgroundImage);
    combineImages(dst, m_foregroundImage);

    return dst;
  } else {
    QImage dst(m_originalBackgroundImage);

    {
      BinaryImage backgroundMask = BinaryImage(m_originalBackgroundImage, BinaryThreshold(1));
      combineImages(dst, m_backgroundImage, backgroundMask);
    }
    {
      BinaryImage foregroundMask = BinaryImage(m_originalBackgroundImage, BinaryThreshold(255)).inverted();
      combineImages(dst, (m_mask.isNull()) ? m_foregroundImage : m_backgroundImage, foregroundMask);
    }

    return dst;
  }
}

QImage SplitImage::getForegroundImage() const {
  if (!m_mask.isNull()) {
    QImage foreground(m_backgroundImage);
    applyMask(foreground, m_mask);

    if (m_isBinaryForeground) {
      foreground = foreground.convertToFormat(QImage::Format_Mono);
    } else if (m_isIndexedForeground) {
      foreground = ColorTable(foreground).toIndexedImage();
    }

    return foreground;
  }

  return m_foregroundImage;
}

void SplitImage::setForegroundImage(const QImage& foregroundImage) {
  m_mask = BinaryImage();
  SplitImage::m_foregroundImage = foregroundImage;
}

QImage SplitImage::getBackgroundImage() const {
  if (!m_mask.isNull()) {
    QImage background(m_backgroundImage);
    applyMask(background, m_mask.inverted());

    return background;
  }

  return m_backgroundImage;
}

void SplitImage::setBackgroundImage(const QImage& backgroundImage) {
  SplitImage::m_backgroundImage = backgroundImage;
}

void SplitImage::applyToLayerImages(const std::function<void(QImage&)>& consumer) {
  if (!m_foregroundImage.isNull()) {
    consumer(m_foregroundImage);
  }
  if (!m_backgroundImage.isNull()) {
    consumer(m_backgroundImage);
  }
  if (!m_originalBackgroundImage.isNull()) {
    consumer(m_originalBackgroundImage);
  }
}

bool SplitImage::isNull() const {
  return (m_foregroundImage.isNull() && m_mask.isNull()) || m_backgroundImage.isNull();
}

void SplitImage::setMask(const BinaryImage& mask, bool binaryForeground) {
  m_foregroundImage = QImage();
  SplitImage::m_mask = mask;
  SplitImage::m_isBinaryForeground = binaryForeground;
}

const QImage& SplitImage::getOriginalBackgroundImage() const {
  return m_originalBackgroundImage;
}

void SplitImage::setOriginalBackgroundImage(const QImage& originalBackgroundImage) {
  SplitImage::m_originalBackgroundImage = originalBackgroundImage;
}

void SplitImage::setIndexedForeground(bool indexedForeground) {
  SplitImage::m_isIndexedForeground = indexedForeground;
}
}  // namespace output
