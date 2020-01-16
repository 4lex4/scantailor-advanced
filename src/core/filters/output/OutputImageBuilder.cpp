// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "OutputImageBuilder.h"

#include <imageproc/BinaryImage.h>

#include "ForegroundType.h"
#include "OutputImageWithOriginalBackgroundMask.h"

using namespace imageproc;

namespace output {
OutputImageBuilder::~OutputImageBuilder() = default;

std::unique_ptr<OutputImage> OutputImageBuilder::build() {
  if (m_image && m_foregroundMask && m_foregroundType && m_backgroundMask) {
    return std::make_unique<OutputImageWithOriginalBackgroundMask>(*m_image, *m_foregroundMask, *m_foregroundType,
                                                                   *m_backgroundMask);
  } else if (m_image && m_foregroundMask && m_foregroundType) {
    return std::make_unique<OutputImageWithForegroundMask>(*m_image, *m_foregroundMask, *m_foregroundType);
  }

  if (m_foregroundImage && m_backgroundImage && m_originalBackgroundImage) {
    return OutputImageWithOriginalBackgroundMask::fromPlainData(*m_foregroundImage, *m_backgroundImage,
                                                                *m_originalBackgroundImage);
  } else if (m_foregroundImage && m_backgroundImage) {
    return OutputImageWithForegroundMask::fromPlainData(*m_foregroundImage, *m_backgroundImage);
  }

  if (m_image) {
    return std::make_unique<OutputImagePlain>(*m_image);
  }

  throw std::invalid_argument("OutputImageBuilder::build(): invalid arguments.");
}

OutputImageBuilder& OutputImageBuilder::setImage(const QImage& image) {
  m_image = std::make_unique<QImage>(image);
  return *this;
}

OutputImageBuilder& OutputImageBuilder::setForegroundImage(const QImage& foregroundImage) {
  m_foregroundImage = std::make_unique<QImage>(foregroundImage);
  return *this;
}

OutputImageBuilder& OutputImageBuilder::setBackgroundImage(const QImage& backgroundImage) {
  m_backgroundImage = std::make_unique<QImage>(backgroundImage);
  return *this;
}

OutputImageBuilder& OutputImageBuilder::setOriginalBackgroundImage(const QImage& originalBackgroundImage) {
  m_originalBackgroundImage = std::make_unique<QImage>(originalBackgroundImage);
  return *this;
}

OutputImageBuilder& OutputImageBuilder::setForegroundMask(const BinaryImage& foregroundMask) {
  m_foregroundMask = std::make_unique<BinaryImage>(foregroundMask);
  return *this;
}

OutputImageBuilder& OutputImageBuilder::setBackgroundMask(const BinaryImage& backgroundMask) {
  m_backgroundMask = std::make_unique<BinaryImage>(backgroundMask);
  return *this;
}

OutputImageBuilder& OutputImageBuilder::setForegroundType(const ForegroundType& foregroundImage) {
  m_foregroundType = std::make_unique<ForegroundType>(foregroundImage);
  return *this;
}
}  // namespace output