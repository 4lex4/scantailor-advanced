// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef IMAGEPROC_REDUCETHRESHOLD_H_
#define IMAGEPROC_REDUCETHRESHOLD_H_

#include "BinaryImage.h"

namespace imageproc {
/**
 * \brief Performs 2x horizontal and vertical downscaling on 1-bit images.
 *
 * The dimensions of the target image will be:
 * \code
 * dst_width = max(1, std::floor(src_width / 2));
 * dst_height = max(1, std::floor(src_height / 2));
 * \endcode
 * \n
 * Processing a null image results in a null image.
 * Processing a non-null image never results in a null image.\n
 * \n
 * A 2x2 square in the source image becomes 1 pixel in the target image.
 * The threshold parameter controls how many black pixels need to be in
 * the source 2x2 square in order to make the destination pixel black.
 * Valid threshold values are 1, 2, 3 and 4.\n
 * \n
 * If the source image is a line 1 pixel thick, downscaling will be done
 * as if the line was thickened to 2 pixels by duplicating existing pixels.\n
 * \n
 * It is possible to do a cascade of reductions:
 * \code
 * BinaryImage out = ReduceThreshold(input)(4)(4)(3);
 * \endcode
 */
class ReduceThreshold {
 public:
  /**
   * \brief Constructor.  Doesn't do any work by itself.
   */
  explicit ReduceThreshold(const BinaryImage& image);

  /**
   * \brief Implicit conversion to BinaryImage.
   */
  operator const BinaryImage&() const { return m_image; }

  /**
   * \brief Returns a reference to the reduced image.
   */
  const BinaryImage& image() const { return m_image; }

  /**
   * \brief Performs a reduction and returns *this.
   */
  ReduceThreshold& reduce(int threshold);

  /**
   * \brief Operator () performs a reduction and returns *this.
   */
  ReduceThreshold& operator()(int threshold) { return reduce(threshold); }

 private:
  void reduceHorLine(int threshold);

  void reduceVertLine(int threshold);

  /**
   * \brief The result of a previous reduction.
   */
  BinaryImage m_image;
};
}  // namespace imageproc
#endif  // ifndef IMAGEPROC_REDUCETHRESHOLD_H_
