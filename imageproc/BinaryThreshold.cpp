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

#include "BinaryThreshold.h"
#include <QDebug>
#include <cassert>
#include <iostream>
#include "Grayscale.h"
#include "Morphology.h"

namespace imageproc {
BinaryThreshold BinaryThreshold::otsuThreshold(const QImage& image) {
  return otsuThreshold(GrayscaleHistogram(image));
}

BinaryThreshold BinaryThreshold::otsuThreshold(const GrayscaleHistogram& pixels_by_color) {
  int32_t pixels_by_threshold[256];
  int64_t moment_by_threshold[256];

  // Note that although BinaryThreshold is defined in such a way
  // that everything below the threshold is considered black,
  // this algorithm assumes that everything below *or equal* to
  // the threshold is considered black.
  // That is, pixels_by_threshold[10] holds the number of pixels
  // in the image that have a gray_level <= 10

  pixels_by_threshold[0] = pixels_by_color[0];
  moment_by_threshold[0] = 0;
  for (int i = 1; i < 256; ++i) {
    pixels_by_threshold[i] = pixels_by_threshold[i - 1] + pixels_by_color[i];
    moment_by_threshold[i] = moment_by_threshold[i - 1] + int64_t(pixels_by_color[i]) * i;
  }

  const int total_pixels = pixels_by_threshold[255];
  const int64_t total_moment = moment_by_threshold[255];
  double max_variance = 0.0;
  int first_best_threshold = -1;
  int last_best_threshold = -1;
  for (int i = 0; i < 256; ++i) {
    const int pixels_below = pixels_by_threshold[i];
    const int pixels_above = total_pixels - pixels_below;
    if ((pixels_below > 0) && (pixels_above > 0)) {  // prevent division by zero
      const int64_t moment_below = moment_by_threshold[i];
      const int64_t moment_above = total_moment - moment_below;
      const double mean_below = (double) moment_below / pixels_below;
      const double mean_above = (double) moment_above / pixels_above;
      const double mean_diff = mean_below - mean_above;
      const double variance = mean_diff * mean_diff * pixels_below * pixels_above;
      if (variance > max_variance) {
        max_variance = variance;
        first_best_threshold = i;
        last_best_threshold = i;
      } else if (variance == max_variance) {
        last_best_threshold = i;
      }
    }
  }

  // Compensate the "< threshold" vs "<= threshold" difference.
  ++first_best_threshold;
  ++last_best_threshold;

  // The middle between the two.
  return BinaryThreshold((first_best_threshold + last_best_threshold) >> 1);
}  // BinaryThreshold::otsuThreshold

BinaryThreshold BinaryThreshold::peakThreshold(const QImage& image) {
  return peakThreshold(GrayscaleHistogram(image));
}

BinaryThreshold BinaryThreshold::peakThreshold(const GrayscaleHistogram& pixels_by_color) {
  int ri = 255, li = 0;
  int right_peak = pixels_by_color[ri];
  int left_peak = pixels_by_color[li];

  for (int i = 254; i >= 0; --i) {
    if (pixels_by_color[i] <= right_peak) {
      if (double(pixels_by_color[i]) < (double(right_peak) * 0.66)) {
        break;
      }
      continue;
    }
    if (pixels_by_color[i] > right_peak) {
      right_peak = pixels_by_color[i];
      ri = i;
    }
  }

  for (int i = 1; i <= 255; ++i) {
    if (pixels_by_color[i] <= left_peak) {
      if (double(pixels_by_color[i]) < (double(left_peak) * 0.66)) {
        break;
      }
      continue;
    }
    if (pixels_by_color[i] > left_peak) {
      left_peak = pixels_by_color[i];
      li = i;
    }
  }

  auto threshold = static_cast<int>(li + (ri - li) * 0.75);

#ifdef DEBUG
  int otsuThreshold = BinaryThreshold::otsuThreshold(pixels_by_color);
  std::cout << "li: " << li << " leftPeak: " << left_peak << std::endl;
  std::cout << "ri: " << ri << " rightPeak: " << right_peak << std::endl;
  std::cout << "threshold: " << threshold << std::endl;
  std::cout << "otsuThreshold: " << otsuThreshold << std::endl;
#endif

  return BinaryThreshold(threshold);
}  // BinaryThreshold::peakThreshold

BinaryThreshold BinaryThreshold::mokjiThreshold(const QImage& image,
                                                const unsigned max_edge_width,
                                                const unsigned min_edge_magnitude) {
  if (max_edge_width < 1) {
    throw std::invalid_argument("mokjiThreshold: invalud max_edge_width");
  }
  if (min_edge_magnitude < 1) {
    throw std::invalid_argument("mokjiThreshold: invalid min_edge_magnitude");
  }

  const GrayImage gray(image);

  const int dilate_size = (max_edge_width + 1) * 2 - 1;
  GrayImage dilated(dilateGray(gray, QSize(dilate_size, dilate_size)));

  unsigned matrix[256][256];
  memset(matrix, 0, sizeof(matrix));

  const int w = image.width();
  const int h = image.height();
  const unsigned char* src_line = gray.data();
  const int src_stride = gray.stride();
  const unsigned char* dilated_line = dilated.data();
  const int dilated_stride = dilated.stride();

  src_line += max_edge_width * src_stride;
  dilated_line += max_edge_width * dilated_stride;
  for (int y = max_edge_width; y < h - (int) max_edge_width; ++y) {
    for (int x = max_edge_width; x < w - (int) max_edge_width; ++x) {
      const unsigned pixel = src_line[x];
      const unsigned darkest_neighbor = dilated_line[x];
      assert(darkest_neighbor <= pixel);

      ++matrix[darkest_neighbor][pixel];
    }
    src_line += src_stride;
    dilated_line += dilated_stride;
  }

  unsigned nominator = 0;
  unsigned denominator = 0;
  for (unsigned m = 0; m < 256 - min_edge_magnitude; ++m) {
    for (unsigned n = m + min_edge_magnitude; n < 256; ++n) {
      assert(n >= m);

      const unsigned val = matrix[m][n];
      nominator += (m + n) * val;
      denominator += val;
    }
  }

  if (denominator == 0) {
    return BinaryThreshold(128);
  }

  const double threshold = 0.5 * nominator / denominator;

  return BinaryThreshold((int) (threshold + 0.5));
}  // BinaryThreshold::mokjiThreshold
}  // namespace imageproc