// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "OutputImageWithForegroundMask.h"
#include <imageproc/Posterizer.h>
#include <imageproc/ImageCombination.h>

using namespace imageproc;

namespace output {
OutputImageWithForegroundMask::OutputImageWithForegroundMask(const QImage& image,
                                                             const imageproc::BinaryImage& mask,
                                                             ForegroundType type)
    : OutputImagePlain(image), m_foregroundMask(mask), m_foregroundType(type) {}

bool OutputImageWithForegroundMask::isNull() const {
  return OutputImagePlain::isNull() || m_foregroundMask.isNull();
}

QImage OutputImageWithForegroundMask::getForegroundImage() const {
  QImage foreground = OutputImagePlain::toImage();
  applyMask(foreground, m_foregroundMask);

  switch (m_foregroundType) {
    case ForegroundType::BINARY:
      foreground = foreground.convertToFormat(QImage::Format_Mono);
      break;
    case ForegroundType::INDEXED:
      foreground = Posterizer::convertToIndexed(foreground);
      break;
    case ForegroundType::COLOR:
      break;
  }

  return foreground;
}

QImage OutputImageWithForegroundMask::getBackgroundImage() const {
  QImage background = OutputImagePlain::toImage();
  applyMask(background, m_foregroundMask.inverted());
  return background;
}

std::unique_ptr<OutputImageWithForegroundMask> OutputImageWithForegroundMask::fromPlainData(
    const QImage& foregroundImage,
    const QImage& backgroundImage) {
  if (foregroundImage.isNull() || backgroundImage.isNull()) {
    return std::make_unique<OutputImageWithForegroundMask>();
  }

  BinaryImage foregroundMask = BinaryImage(backgroundImage, BinaryThreshold(255)).inverted();

  QImage image(backgroundImage);
  combineImages(image, foregroundImage);

  return std::make_unique<OutputImageWithForegroundMask>(image, foregroundMask, getForegroundType(foregroundImage));
}

ForegroundType OutputImageWithForegroundMask::getForegroundType(
    const QImage& foregroundImage) {
  if ((foregroundImage.format() == QImage::Format_Mono) || (foregroundImage.format() == QImage::Format_MonoLSB)) {
    return ForegroundType::BINARY;
  } else if ((foregroundImage.format() == QImage::Format_Indexed8) && !foregroundImage.isGrayscale()) {
    return ForegroundType::INDEXED;
  } else {
    return ForegroundType::COLOR;
  }
}
}  // namespace output
