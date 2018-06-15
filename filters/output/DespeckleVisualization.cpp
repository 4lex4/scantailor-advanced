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

#include "DespeckleVisualization.h"
#include <QPainter>
#include "Dpi.h"
#include "ImageViewBase.h"
#include "imageproc/BinaryImage.h"
#include "imageproc/SEDM.h"

using namespace imageproc;

namespace output {
DespeckleVisualization::DespeckleVisualization(const QImage& output,
                                               const imageproc::BinaryImage& speckles,
                                               const Dpi& dpi) {
  if (output.isNull()) {
    // This can happen in batch processing mode.
    return;
  }

  m_image = output.convertToFormat(QImage::Format_RGB32);

  if (!speckles.isNull()) {
    colorizeSpeckles(m_image, speckles, dpi);
  }

  m_downscaledImage = ImageViewBase::createDownscaledImage(m_image);
}

void DespeckleVisualization::colorizeSpeckles(QImage& image, const imageproc::BinaryImage& speckles, const Dpi& dpi) {
  const int w = image.width();
  const int h = image.height();
  auto* image_line = (uint32_t*) image.bits();
  const int image_stride = image.bytesPerLine() / 4;

  const SEDM sedm(speckles, SEDM::DIST_TO_BLACK, SEDM::DIST_TO_NO_BORDERS);
  const uint32_t* sedm_line = sedm.data();
  const int sedm_stride = sedm.stride();

  const float radius = static_cast<const float>(15.0 * std::max(dpi.horizontal(), dpi.vertical()) / 600);
  const float sq_radius = radius * radius;

  for (int y = 0; y < h; ++y) {
    for (int x = 0; x < w; ++x) {
      const uint32_t sq_dist = sedm_line[x];
      if (sq_dist == 0) {
        // Speckle pixel.
        image_line[x] = 0xffff0000;  // opaque red
        continue;
      } else if ((image_line[x] & 0x00ffffff) == 0x0) {
        // Non-speckle black pixel.
        continue;
      }

      const float alpha_upper_bound = 0.8f;
      const float scale = alpha_upper_bound / sq_radius;
      const float alpha = alpha_upper_bound - scale * sq_dist;
      if (alpha > 0) {
        const float alpha2 = 1.0f - alpha;
        const float overlay_r = 255;
        const float overlay_g = 0;
        const float overlay_b = 0;
        const float r = overlay_r * alpha + qRed(image_line[x]) * alpha2;
        const float g = overlay_g * alpha + qGreen(image_line[x]) * alpha2;
        const float b = overlay_b * alpha + qBlue(image_line[x]) * alpha2;
        image_line[x] = qRgb(int(r), int(g), int(b));
      }
    }
    sedm_line += sedm_stride;
    image_line += image_stride;
  }
}

bool DespeckleVisualization::isNull() const {
  return m_image.isNull();
}

const QImage& DespeckleVisualization::image() const {
  return m_image;
}

const QImage& DespeckleVisualization::downscaledImage() const {
  return m_downscaledImage;
}
}  // namespace output