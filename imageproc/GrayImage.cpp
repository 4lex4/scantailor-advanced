/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C)  Joseph Artsimovich <joseph.artsimovich@gmail.com>

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

#include "GrayImage.h"
#include "Grayscale.h"

namespace imageproc {
GrayImage::GrayImage(QSize size) {
  if (size.isEmpty()) {
    return;
  }

  m_image = QImage(size, QImage::Format_Indexed8);
  m_image.setColorTable(createGrayscalePalette());
  if (m_image.isNull()) {
    throw std::bad_alloc();
  }
}

GrayImage::GrayImage(const QImage& image) : m_image(toGrayscale(image)) {}

GrayImage GrayImage::inverted() const {
  GrayImage inverted(*this);
  inverted.invert();

  return inverted;
}

void GrayImage::invert() {
  m_image.invertPixels(QImage::InvertRgb);
}

int GrayImage::dotsPerMeterX() const {
  return m_image.dotsPerMeterX();
}

int GrayImage::dotsPerMeterY() const {
  return m_image.dotsPerMeterY();
}

void GrayImage::setDotsPerMeterX(int value) {
  m_image.setDotsPerMeterX(value);
}

void GrayImage::setDotsPerMeterY(int value) {
  m_image.setDotsPerMeterY(value);
}
}  // namespace imageproc
