// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "DespeckleVisualization.h"

#include <BinaryImage.h>
#include <SEDM.h>

#include <QPainter>

#include "Dpi.h"
#include "ImageViewBase.h"

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
  auto* imageLine = (uint32_t*) image.bits();
  const int imageStride = image.bytesPerLine() / 4;

  const SEDM sedm(speckles, SEDM::DIST_TO_BLACK, SEDM::DIST_TO_NO_BORDERS);
  const uint32_t* sedmLine = sedm.data();
  const int sedmStride = sedm.stride();

  const float radius = static_cast<float>(45.0 * std::max(dpi.horizontal(), dpi.vertical()) / 600);
  const float sqRadius = radius * radius;

  for (int y = 0; y < h; ++y) {
    for (int x = 0; x < w; ++x) {
      const uint32_t sqDist = sedmLine[x];
      if (sqDist == 0) {
        // Speckle pixel.
        imageLine[x] = 0xffff0000;  // opaque red
        continue;
      } else if ((imageLine[x] & 0x00ffffff) == 0x0) {
        // Non-speckle black pixel.
        continue;
      }

      const float alphaUpperBound = 0.7f;
      const float noSpecklesOverlayAlpha = 0.3f;
      const float scale = alphaUpperBound / sqRadius;
      const float alpha = (sqDist == SEDM::INF_DIST // check if there are any speckles
                           ? noSpecklesOverlayAlpha
                           : alphaUpperBound - scale * sqDist);
      if (alpha > 0) {
        const float alpha2 = 1.0f - alpha;
        const float overlayR = 255;
        const float overlayG = 0;
        const float overlayB = 0;
        const float r = overlayR * alpha + qRed(imageLine[x]) * alpha2;
        const float g = overlayG * alpha + qGreen(imageLine[x]) * alpha2;
        const float b = overlayB * alpha + qBlue(imageLine[x]) * alpha2;
        imageLine[x] = qRgb(int(r), int(g), int(b));
      }
    }
    sedmLine += sedmStride;
    imageLine += imageStride;
  }
}

bool DespeckleVisualization::isNull() const {
  return m_image.isNull();
}
}  // namespace output
