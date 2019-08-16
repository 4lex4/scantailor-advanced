// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef IMAGEPROC_BINARYTHRESHOLD_H_
#define IMAGEPROC_BINARYTHRESHOLD_H_

#include "BWColor.h"

class QImage;

namespace imageproc {
class GrayscaleHistogram;

/**
 * \brief Defines the gray level threshold that separates black from white.
 *
 * Gray levels in range of [0, threshold) are considered black, while
 * levels in range of [threshold, 255] are considered white.  The threshold
 * itself is considered to be white.
 */
class BinaryThreshold {
  // Member-wise copying is OK.
 public:
  /**
   * \brief Finds the threshold using Otsu’s thresholding method.
   */
  static BinaryThreshold otsuThreshold(const QImage& image);

  /**
   * \brief Finds the threshold using Otsu’s thresholding method.
   */
  static BinaryThreshold otsuThreshold(const GrayscaleHistogram& pixels_by_color);

  static BinaryThreshold peakThreshold(const QImage& image);

  static BinaryThreshold peakThreshold(const GrayscaleHistogram& pixels_by_color);

  /**
   * \brief Image binarization using Mokji's global thresholding method.
   *
   * M. M. Mokji, S. A. R. Abu-Bakar: Adaptive Thresholding Based on
   * Co-occurrence Matrix Edge Information. Asia International Conference on
   * Modelling and Simulation 2007: 444-450
   * http://www.academypublisher.com/jcp/vol02/no08/jcp02084452.pdf
   *
   * \param image The source image.  May be in any format.
   * \param max_edge_width The maximum gradient length to consider.
   * \param min_edge_magnitude The minimum color difference in a gradient.
   * \return A black and white image.
   */
  static BinaryThreshold mokjiThreshold(const QImage& image,
                                        unsigned max_edge_width = 3,
                                        unsigned min_edge_magnitude = 20);

  BinaryThreshold(int threshold) : m_threshold(threshold) {}

  operator int() const { return m_threshold; }

  BWColor grayToBW(int gray) const { return gray < m_threshold ? BLACK : WHITE; }

 private:
  int m_threshold;
};
}  // namespace imageproc
#endif  // ifndef IMAGEPROC_BINARYTHRESHOLD_H_
