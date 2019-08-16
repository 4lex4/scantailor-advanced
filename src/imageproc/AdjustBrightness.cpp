// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "AdjustBrightness.h"
#include <QImage>

namespace imageproc {
void adjustBrightness(QImage& rgb_image, const QImage& brightness, const double wr, const double wb) {
  switch (rgb_image.format()) {
    case QImage::Format_RGB32:
    case QImage::Format_ARGB32:
      break;
    default:
      throw std::invalid_argument("adjustBrightness: not (A)RGB32");
  }

  if ((brightness.format() != QImage::Format_Indexed8) || !brightness.isGrayscale()) {
    throw std::invalid_argument("adjustBrightness: brightness not grayscale");
  }

  if (rgb_image.size() != brightness.size()) {
    throw std::invalid_argument("adjustBrightness: image and brightness have different sizes");
  }

  auto* rgb_line = reinterpret_cast<uint32_t*>(rgb_image.bits());
  const int rgb_wpl = rgb_image.bytesPerLine() / 4;

  const uint8_t* br_line = brightness.bits();
  const int br_bpl = brightness.bytesPerLine();

  const int width = rgb_image.width();
  const int height = rgb_image.height();

  const double wg = 1.0 - wr - wb;
  const double wu = (1.0 - wb);
  const double wv = (1.0 - wr);
  const double r_wg = 1.0 / wg;
  const double r_wu = 1.0 / wu;
  const double r_wv = 1.0 / wv;

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      uint32_t RGB = rgb_line[x];
      const double R = (RGB >> 16) & 0xFF;
      const double G = (RGB >> 8) & 0xFF;
      const double B = RGB & 0xFF;

      const double Y = wr * R + wg * G + wb * B;
      const double U = (B - Y) * r_wu;
      const double V = (R - Y) * r_wv;

      double new_Y = br_line[x];
      double new_R = new_Y + V * wv;
      double new_B = new_Y + U * wu;
      double new_G = (new_Y - new_R * wr - new_B * wb) * r_wg;

      RGB &= 0xFF000000;  // preserve alpha
      RGB |= uint32_t(qBound(0, int(new_R + 0.5), 255)) << 16;
      RGB |= uint32_t(qBound(0, int(new_G + 0.5), 255)) << 8;
      RGB |= uint32_t(qBound(0, int(new_B + 0.5), 255));
      rgb_line[x] = RGB;
    }
    rgb_line += rgb_wpl;
    br_line += br_bpl;
  }
}  // adjustBrightness

void adjustBrightnessYUV(QImage& rgb_image, const QImage& brightness) {
  adjustBrightness(rgb_image, brightness, 0.299, 0.114);
}

void adjustBrightnessGrayscale(QImage& rgb_image, const QImage& brightness) {
  adjustBrightness(rgb_image, brightness, 11.0 / 32.0, 5.0 / 32.0);
}
}  // namespace imageproc