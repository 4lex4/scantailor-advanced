/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2007-2008  Joseph Artsimovich <joseph_a@mail.ru>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "FilterData.h"
#include "Dpm.h"
#include "imageproc/Grayscale.h"

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

const ImageTransformation& FilterData::xform() const {
  return m_xform;
}

const QImage& FilterData::origImage() const {
  return m_origImage;
}

const imageproc::GrayImage& FilterData::grayImage() const {
  return m_grayImage;
}

bool FilterData::isBlackOnWhite() const {
  return m_imageParams.isBlackOnWhite();
}

void FilterData::updateImageParams(const ImageSettings::PageParams& imageParams) {
  m_imageParams = imageParams;
}
