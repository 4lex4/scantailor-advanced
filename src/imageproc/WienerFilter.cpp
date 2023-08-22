/*
  Scan Tailor - Interactive post-processing tool for scanned pages.
  Copyright (C) 2015  Joseph Artsimovich <joseph.artsimovich@gmail.com>

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

#include "WienerFilter.h"

#include <QSize>
#include <QtGlobal>
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <stdexcept>

#include "GrayImage.h"
#include "IntegralImage.h"

namespace imageproc {

GrayImage wienerFilter(GrayImage const& image, QSize const& window_size, double const noise_sigma) {
  GrayImage dst(image);
  wienerFilterInPlace(dst, window_size, noise_sigma);
  return dst;
}

void wienerFilterInPlace(GrayImage& image, QSize const& window_size, double const noise_sigma) {
  if (window_size.isEmpty()) {
    throw std::invalid_argument("wienerFilter: empty window_size");
  }
  if (noise_sigma < 0) {
    throw std::invalid_argument("wienerFilter: negative noise_sigma");
  }
  if (image.isNull()) {
    return;
  }

  int const w = image.width();
  int const h = image.height();
  double const noise_variance = noise_sigma * noise_sigma;

  IntegralImage<uint32_t> integral_image(w, h);
  IntegralImage<uint64_t> integral_sqimage(w, h);

  uint8_t* image_line = image.data();
  int const image_stride = image.stride();

  for (int y = 0; y < h; ++y) {
    integral_image.beginRow();
    integral_sqimage.beginRow();
    for (int x = 0; x < w; ++x) {
      uint32_t const pixel = image_line[x];
      integral_image.push(pixel);
      integral_sqimage.push(pixel * pixel);
    }
    image_line += image_stride;
  }

  int const window_lower_half = window_size.height() >> 1;
  int const window_upper_half = window_size.height() - window_lower_half;
  int const window_left_half = window_size.width() >> 1;
  int const window_right_half = window_size.width() - window_left_half;

  image_line = image.data();
  for (int y = 0; y < h; ++y) {
    int const top = ((y - window_lower_half) < 0) ? 0 : (y - window_lower_half);
    int const bottom = ((y + window_upper_half) < h) ? (y + window_upper_half) : h;  // exclusive

    for (int x = 0; x < w; ++x) {
      int const left = ((x - window_left_half) < 0) ? 0 : (x - window_left_half);
      int const right = ((x + window_right_half) < w) ? (x + window_right_half) : w;  // exclusive
      int const area = (bottom - top) * (right - left);
      assert(area > 0);  // because window_size > 0 and w > 0 and h > 0

      QRect const rect(left, top, right - left, bottom - top);
      double const window_sum = integral_image.sum(rect);
      double const window_sqsum = integral_sqimage.sum(rect);

      double const r_area = 1.0 / area;
      double const mean = window_sum * r_area;
      double const sqmean = window_sqsum * r_area;
      double const variance = sqmean - mean * mean;

      if (variance > 1e-6) {
        double const src_pixel = (double) image_line[x];
        double const dst_pixel = mean
                                 + (src_pixel - mean)
                                       * (((variance - noise_variance) < 0.0) ? 0.0 : (variance - noise_variance))
                                       / variance;
        image_line[x] = (uint8_t)((dst_pixel < 0.0) ? 0.0 : ((dst_pixel < 255.0) ? dst_pixel : 255.0));
      }
    }
    image_line += image_stride;
  }
}

QImage wienerColorFilter(QImage const& image, QSize const& window_size, double const coef) {
  QImage dst(image);
  wienerColorFilterInPlace(dst, window_size, coef);
  return dst;
}

void wienerColorFilterInPlace(QImage& image, QSize const& window_size, double const coef) {
  if (image.isNull()) {
    return;
  }
  if (window_size.isEmpty()) {
    throw std::invalid_argument("wienerFilter: empty window_size");
  }

  if (coef > 0.0) {
    int const w = image.width();
    int const h = image.height();
    uint8_t* image_line = (uint8_t*) image.bits();
    int const image_bpl = image.bytesPerLine();
    unsigned int const cnum = image_bpl / w;

    GrayImage gray = GrayImage(image);
    uint8_t* gray_line = gray.data();
    int const gray_bpl = gray.stride();
    GrayImage wiener(wienerFilter(gray, window_size, 255.0 * coef));
    uint8_t* wiener_line = wiener.data();
    int const wiener_bpl = wiener.stride();

    for (int y = 0; y < h; ++y) {
      for (int x = 0; x < w; ++x) {
        float const origin = gray_line[x];
        float color = wiener_line[x];

        float const colscale = (color + 1.0f) / (origin + 1.0f);
        float const coldelta = color - origin * colscale;
        for (unsigned int c = 0; c < cnum; ++c) {
          int const indx = x * cnum + c;
          float origcol = image_line[indx];
          float val = origcol * colscale + coldelta;
          val = (val < 0.0f) ? 0.0f : (val < 255.0f) ? val : 255.0f;
          image_line[indx] = (uint8_t)(val + 0.5f);
        }
      }
      image_line += image_bpl;
      gray_line += gray_bpl;
      wiener_line += wiener_bpl;
    }
  }
}

}  // namespace imageproc
