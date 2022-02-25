// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_IMAGEPROC_SLICEDHISTOGRAM_H_
#define SCANTAILOR_IMAGEPROC_SLICEDHISTOGRAM_H_

#include <cstddef>
#include <vector>
#include <stdexcept>

class QRect;

namespace imageproc {
class BinaryImage;

/**
 * \brief Calculates and stores the number of black pixels
 *        in each horizontal or vertical line.
 */
class SlicedHistogram {
  // Member-wise copying is OK.
 public:
  enum Type {
    ROWS, /**< Process horizontal lines. */
    COLS  /**< Process vertical lines. */
  };

  /**
   * \brief Constructs an empty histogram.
   */
  SlicedHistogram();

  /**
   * \brief Calculates the histogram of the whole image.
   *
   * \param image The image to process.  A null image will produce
   *        an empty histogram.
   * \param type Specifies whether to process columns or rows.
   */
  SlicedHistogram(const BinaryImage& image, Type type);

  /**
   * \brief Calculates the histogram of a portion of the image.
   *
   * \param image The image to process.  A null image will produce
   *        an empty histogram, provided that \p area is also null.
   * \param area The area of the image to process.  The first value
   *        in the histogram will correspond to the first line in this area.
   * \param type Specifies whether to process columns or rows.
   *
   * \exception std::invalid_argument If \p area is not completely
   *            within image.rect().
   */
  SlicedHistogram(const BinaryImage& image, const QRect& area, Type type);

  size_t size() const { return m_data.size(); }

  void setSize(size_t size) { m_data.resize(size); }

  const int& operator[](size_t idx) const { return m_data[idx]; }

  int& operator[](size_t idx) { return m_data[idx]; }

 private:
  void processHorizontalLines(const BinaryImage& image, const QRect& area);

  void processVerticalLines(const BinaryImage& image, const QRect& area);

  std::vector<int> m_data;
};
}  // namespace imageproc
#endif  // ifndef SCANTAILOR_IMAGEPROC_SLICEDHISTOGRAM_H_
