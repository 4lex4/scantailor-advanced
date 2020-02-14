// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "FilterData.h"

#include <Grayscale.h>

#include "Dpm.h"

using namespace imageproc;

FilterData::FilterData(const QImage& image)
    : m_origImage(image), m_grayImage(toGrayscale(m_origImage)), m_xform(image.rect(), Dpm(image)) {}

FilterData::FilterData(const FilterData& other, const ImageTransformation& xform)
    : m_origImage(other.m_origImage),
      m_grayImage(other.m_grayImage),
      m_xform(xform),
      m_imageParams(other.m_imageParams) {}

FilterData::FilterData(const FilterData& other) = default;

imageproc::BinaryThreshold FilterData::bwThreshold() const {
  return m_imageParams.getBwThreshold();
}

bool FilterData::isBlackOnWhite() const {
  return m_imageParams.isBlackOnWhite();
}

imageproc::BinaryThreshold FilterData::bwThresholdBlackOnWhite() const {
  return isBlackOnWhite() ? bwThreshold() : BinaryThreshold(256 - int(bwThreshold()));
}

imageproc::GrayImage FilterData::grayImageBlackOnWhite() const {
  return isBlackOnWhite() ? m_grayImage : m_grayImage.inverted();
}
