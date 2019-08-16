// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef IMAGEPROC_INTEGRALIMAGE_H_
#define IMAGEPROC_INTEGRALIMAGE_H_

#include <QRect>
#include <QSize>
#include <new>
#include "NonCopyable.h"

namespace imageproc {
/**
 * \brief Provides the sum of values in a sub-rectangle of a 2D array in constant time.
 *
 * Suppose you have a MxN array of some values.  Now if we are not going to
 * alter it, but are going to calculate the sum of values in various
 * rectangular sub-regions, it's best to use an integral image for this purpose.
 * We construct it once, with complexity of O(M*N), and then obtain the sum
 * of values in a rectangular sub-region in O(1).
 *
 * \note Template parameter T must be large enough to hold the sum of all
 *       values in the array.
 */
template <typename T>
class IntegralImage {
  DECLARE_NON_COPYABLE(IntegralImage)

 public:
  IntegralImage(int width, int height);

  explicit IntegralImage(const QSize& size);

  ~IntegralImage();

  /**
   * \brief To be called before pushing new row data.
   */
  void beginRow();

  /**
   * \brief Push a single value to the integral image.
   *
   * Values must be pushed row by row, starting from row 0, and from
   * column to column within each row, starting from column 0.
   * At the beginning of a row, a call to beginRow() must be made.
   *
   * \note Pushing more than width * height values results in undefined
   *       behavior.
   */
  void push(T val);

  /**
   * \brief Calculate the sum of values in the given rectangle.
   *
   * \note If the rectangle exceeds the image area, the behaviour is
   *       undefined.
   */
  T sum(const QRect& rect) const;

 private:
  void init(int width, int height);

  T* m_data;
  T* m_cur;
  T* m_above;
  T m_lineSum;
  int m_width;
  int m_height;
};


template <typename T>
IntegralImage<T>::IntegralImage(const int width, const int height)
    : m_lineSum() {  // init with 0 or with default constructor.
  // The first row and column are fake.
  init(width + 1, height + 1);
}

template <typename T>
IntegralImage<T>::IntegralImage(const QSize& size) : m_lineSum() {  // init with 0 or with default constructor.
  // The first row and column are fake.
  init(size.width() + 1, size.height() + 1);
}

template <typename T>
IntegralImage<T>::~IntegralImage() {
  delete[] m_data;
}

template <typename T>
void IntegralImage<T>::init(const int width, const int height) {
  m_width = width;
  m_height = height;

  m_data = new T[width * height];

  // Initialize the first (fake) row.
  // As for the fake column, we initialize its elements in beginRow().
  T* p = m_data;
  for (int i = 0; i < width; ++i, ++p) {
    *p = T();
  }

  m_above = m_data;
  m_cur = m_above + width;  // Skip the first row.
}

template <typename T>
void IntegralImage<T>::push(const T val) {
  m_lineSum += val;
  *m_cur = *m_above + m_lineSum;
  ++m_cur;
  ++m_above;
}

template <typename T>
void IntegralImage<T>::beginRow() {
  m_lineSum = T();

  // Initialize and skip the fake column.
  *m_cur = T();
  ++m_cur;
  ++m_above;
}

template <typename T>
inline T IntegralImage<T>::sum(const QRect& rect) const {
  // Keep in mind that row 0 and column 0 are fake.
  const int pre_left = rect.left();
  const int pre_right = rect.right() + 1;  // QRect::right() is inclusive.
  const int pre_top = rect.top();
  const int pre_bottom = rect.bottom() + 1;  // QRect::bottom() is inclusive.
  T sum(m_data[pre_bottom * m_width + pre_right]);
  sum -= m_data[pre_top * m_width + pre_right];
  sum += m_data[pre_top * m_width + pre_left];
  sum -= m_data[pre_bottom * m_width + pre_left];

  return sum;
}
}  // namespace imageproc
#endif  // ifndef IMAGEPROC_INTEGRALIMAGE_H_
