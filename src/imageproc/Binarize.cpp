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

#include "Binarize.h"
#include <QDebug>
#include <cassert>
#include <cmath>
#include "BinaryImage.h"
#include "Grayscale.h"
#include "IntegralImage.h"

namespace imageproc {
BinaryImage binarizeOtsu(const QImage& src) {
  return BinaryImage(src, BinaryThreshold::otsuThreshold(src));
}

BinaryImage binarizeMokji(const QImage& src, const unsigned max_edge_width, const unsigned min_edge_magnitude) {
  const BinaryThreshold threshold(BinaryThreshold::mokjiThreshold(src, max_edge_width, min_edge_magnitude));

  return BinaryImage(src, threshold);
}

BinaryImage binarizeSauvola(const QImage& src, const QSize window_size, const double k) {
  if (window_size.isEmpty()) {
    throw std::invalid_argument("binarizeSauvola: invalid window_size");
  }

  if (src.isNull()) {
    return BinaryImage();
  }

  const QImage gray(toGrayscale(src));
  const int w = gray.width();
  const int h = gray.height();

  IntegralImage<uint32_t> integral_image(w, h);
  IntegralImage<uint64_t> integral_sqimage(w, h);

  const uint8_t* gray_line = gray.bits();
  const int gray_bpl = gray.bytesPerLine();

  for (int y = 0; y < h; ++y, gray_line += gray_bpl) {
    integral_image.beginRow();
    integral_sqimage.beginRow();
    for (int x = 0; x < w; ++x) {
      const uint32_t pixel = gray_line[x];
      integral_image.push(pixel);
      integral_sqimage.push(pixel * pixel);
    }
  }

  const int window_lower_half = window_size.height() >> 1;
  const int window_upper_half = window_size.height() - window_lower_half;
  const int window_left_half = window_size.width() >> 1;
  const int window_right_half = window_size.width() - window_left_half;

  BinaryImage bw_img(w, h);
  uint32_t* bw_line = bw_img.data();
  const int bw_wpl = bw_img.wordsPerLine();

  gray_line = gray.bits();
  for (int y = 0; y < h; ++y) {
    const int top = std::max(0, y - window_lower_half);
    const int bottom = std::min(h, y + window_upper_half);  // exclusive
    for (int x = 0; x < w; ++x) {
      const int left = std::max(0, x - window_left_half);
      const int right = std::min(w, x + window_right_half);  // exclusive
      const int area = (bottom - top) * (right - left);
      assert(area > 0);  // because window_size > 0 and w > 0 and h > 0
      const QRect rect(left, top, right - left, bottom - top);
      const double window_sum = integral_image.sum(rect);
      const double window_sqsum = integral_sqimage.sum(rect);

      const double r_area = 1.0 / area;
      const double mean = window_sum * r_area;
      const double sqmean = window_sqsum * r_area;

      const double variance = sqmean - mean * mean;
      const double deviation = std::sqrt(std::fabs(variance));

      const double threshold = mean * (1.0 + k * (deviation / 128.0 - 1.0));

      const uint32_t msb = uint32_t(1) << 31;
      const uint32_t mask = msb >> (x & 31);
      if (int(gray_line[x]) < threshold) {
        // black
        bw_line[x >> 5] |= mask;
      } else {
        // white
        bw_line[x >> 5] &= ~mask;
      }
    }

    gray_line += gray_bpl;
    bw_line += bw_wpl;
  }

  return bw_img;
}  // binarizeSauvola

BinaryImage binarizeWolf(const QImage& src,
                         const QSize window_size,
                         const unsigned char lower_bound,
                         const unsigned char upper_bound,
                         const double k) {
  if (window_size.isEmpty()) {
    throw std::invalid_argument("binarizeWolf: invalid window_size");
  }

  if (src.isNull()) {
    return BinaryImage();
  }

  const QImage gray(toGrayscale(src));
  const int w = gray.width();
  const int h = gray.height();

  IntegralImage<uint32_t> integral_image(w, h);
  IntegralImage<uint64_t> integral_sqimage(w, h);

  const uint8_t* gray_line = gray.bits();
  const int gray_bpl = gray.bytesPerLine();

  uint32_t min_gray_level = 255;

  for (int y = 0; y < h; ++y, gray_line += gray_bpl) {
    integral_image.beginRow();
    integral_sqimage.beginRow();
    for (int x = 0; x < w; ++x) {
      const uint32_t pixel = gray_line[x];
      integral_image.push(pixel);
      integral_sqimage.push(pixel * pixel);
      min_gray_level = std::min(min_gray_level, pixel);
    }
  }

  const int window_lower_half = window_size.height() >> 1;
  const int window_upper_half = window_size.height() - window_lower_half;
  const int window_left_half = window_size.width() >> 1;
  const int window_right_half = window_size.width() - window_left_half;

  std::vector<float> means(w * h, 0);
  std::vector<float> deviations(w * h, 0);

  double max_deviation = 0;

  for (int y = 0; y < h; ++y) {
    const int top = std::max(0, y - window_lower_half);
    const int bottom = std::min(h, y + window_upper_half);  // exclusive
    for (int x = 0; x < w; ++x) {
      const int left = std::max(0, x - window_left_half);
      const int right = std::min(w, x + window_right_half);  // exclusive
      const int area = (bottom - top) * (right - left);
      assert(area > 0);  // because window_size > 0 and w > 0 and h > 0
      const QRect rect(left, top, right - left, bottom - top);
      const double window_sum = integral_image.sum(rect);
      const double window_sqsum = integral_sqimage.sum(rect);

      const double r_area = 1.0 / area;
      const double mean = window_sum * r_area;
      const double sqmean = window_sqsum * r_area;

      const double variance = sqmean - mean * mean;
      const double deviation = std::sqrt(std::fabs(variance));
      max_deviation = std::max(max_deviation, deviation);
      means[w * y + x] = (float) mean;
      deviations[w * y + x] = (float) deviation;
    }
  }

  // TODO: integral images can be disposed at this point.

  BinaryImage bw_img(w, h);
  uint32_t* bw_line = bw_img.data();
  const int bw_wpl = bw_img.wordsPerLine();

  gray_line = gray.bits();
  for (int y = 0; y < h; ++y, gray_line += gray_bpl, bw_line += bw_wpl) {
    for (int x = 0; x < w; ++x) {
      const float mean = means[y * w + x];
      const float deviation = deviations[y * w + x];
      const double a = 1.0 - deviation / max_deviation;
      const double threshold = mean - k * a * (mean - min_gray_level);

      const uint32_t msb = uint32_t(1) << 31;
      const uint32_t mask = msb >> (x & 31);
      if ((gray_line[x] < lower_bound) || ((gray_line[x] <= upper_bound) && (int(gray_line[x]) < threshold))) {
        // black
        bw_line[x >> 5] |= mask;
      } else {
        // white
        bw_line[x >> 5] &= ~mask;
      }
    }
  }

  return bw_img;
}  // binarizeWolf

BinaryImage peakThreshold(const QImage& image) {
  return BinaryImage(image, BinaryThreshold::peakThreshold(image));
}
}  // namespace imageproc