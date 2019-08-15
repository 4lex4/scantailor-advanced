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

#ifndef MAT_T_H_
#define MAT_T_H_

#include <boost/scoped_array.hpp>
#include <cassert>
#include <cstddef>

/**
 * \brief A matrix of elements of type T.
 *
 * \note The memory layout is always column-major, as that's what MatrixCalc uses.
 */
template <typename T>
class MatT {
 public:
  typedef T type;

  /**
   * \brief Constructs an empty 0x0 matrix.
   */
  MatT();

  /**
   * \brief Constructs a (rows)x(cols) matrix, initializing all elements to T().
   */
  MatT(size_t rows, size_t cols);

  /**
   * \brief Constructs a (rows)x(cols) matrix, initializing all elements to the provided value.
   */
  MatT(size_t rows, size_t cols, T initial_value);

  /**
   * \brief Construction from an array of elements of possibly different type.
   *
   * Conversion is done by static casts.  Data elements must be in column-major order.
   */
  template <typename OT>
  explicit MatT(size_t rows, size_t cols, const OT* data);

  /**
   * Ordinary copy-construction.
   */
  MatT(const MatT& other);

  /**
   * \brief Construction from a matrix of a different type.
   *
   * Conversion is done by static casts.
   */
  template <typename OT>
  explicit MatT(const MatT<OT>& other);

  /**
   * \brief Ordinary assignment.
   */
  MatT& operator=(const MatT& other);

  /**
   * \brief Assignment from a matrix of a different type.
   *
   * Conversion is done by static casts.
   */
  template <typename OT>
  MatT& operator=(const MatT<OT>& other);

  MatT& operator+=(const MatT& rhs);

  MatT& operator-=(const MatT& rhs);

  MatT& operator*=(T scalar);

  size_t rows() const { return m_rows; }

  size_t cols() const { return m_cols; }

  const T* data() const { return m_data.get(); }

  T* data() { return m_data.get(); }

  const T& operator()(size_t row, size_t col) const {
    assert(row < m_rows && col < m_cols);

    return m_data[row + col * m_rows];
  }

  T& operator()(size_t row, size_t col) {
    assert(row < m_rows && col < m_cols);

    return m_data[row + col * m_rows];
  }

  void fill(const T& value);

  void swap(MatT& other);

 private:
  size_t m_rows;
  size_t m_cols;
  boost::scoped_array<T> m_data;
};


template <typename T>
MatT<T>::MatT() : m_rows(0), m_cols(0) {}

template <typename T>
MatT<T>::MatT(size_t rows, size_t cols)
    : m_rows(rows),
      m_cols(cols),
      m_data(new T[rows * cols]()) {  // The "()" will cause elements to be initialized to T().
}

template <typename T>
MatT<T>::MatT(size_t rows, size_t cols, T initial_value) : m_rows(rows), m_cols(cols), m_data(new T[rows * cols]) {
  const size_t len = rows * cols;
  for (size_t i = 0; i < len; ++i) {
    m_data[i] = initial_value;
  }
}

template <typename T>
template <typename OT>
MatT<T>::MatT(size_t rows, size_t cols, const OT* data) : m_rows(rows), m_cols(cols), m_data(new T[rows * cols]) {
  const size_t len = rows * cols;
  for (size_t i = 0; i < len; ++i) {
    m_data[i] = static_cast<T>(data[i]);
  }
}

template <typename T>
MatT<T>::MatT(const MatT& other) : m_rows(other.rows()), m_cols(other.cols()), m_data(new T[m_rows * m_cols]) {
  const size_t len = m_rows * m_cols;
  const T* other_data = other.data();
  for (size_t i = 0; i < len; ++i) {
    m_data[i] = other_data[i];
  }
}

template <typename T>
template <typename OT>
MatT<T>::MatT(const MatT<OT>& other) : m_rows(other.rows()), m_cols(other.cols()), m_data(new T[m_rows * m_cols]) {
  const size_t len = m_rows * m_cols;
  const T* other_data = other.data();
  for (size_t i = 0; i < len; ++i) {
    m_data[i] = other_data[i];
  }
}

template <typename T>
MatT<T>& MatT<T>::operator=(const MatT& other) {
  MatT(other).swap(*this);

  return *this;
}

template <typename T>
template <typename OT>
MatT<T>& MatT<T>::operator=(const MatT<OT>& other) {
  MatT(other).swap(*this);

  return *this;
}

template <typename T>
MatT<T>& MatT<T>::operator+=(const MatT& rhs) {
  assert(m_rows == rhs.m_rows && m_cols == rhs.m_cols);

  const size_t len = m_rows * m_cols;
  for (size_t i = 0; i < len; ++i) {
    m_data[i] += rhs.m_data[i];
  }

  return *this;
}

template <typename T>
MatT<T>& MatT<T>::operator-=(const MatT& rhs) {
  assert(m_rows == rhs.m_rows && m_cols == rhs.m_cols);

  const size_t len = m_rows * m_cols;
  for (size_t i = 0; i < len; ++i) {
    m_data[i] -= rhs.m_data[i];
  }

  return *this;
}

template <typename T>
MatT<T>& MatT<T>::operator*=(const T scalar) {
  const size_t len = m_rows * m_cols;
  for (size_t i = 0; i < len; ++i) {
    m_data[i] *= scalar;
  }

  return *this;
}

template <typename T>
void MatT<T>::fill(const T& value) {
  const size_t len = m_rows * m_cols;
  for (size_t i = 0; i < len; ++i) {
    m_data[i] = value;
  }
}

template <typename T>
void MatT<T>::swap(MatT& other) {
  size_t tmp = m_rows;
  m_rows = other.m_rows;
  other.m_rows = tmp;

  tmp = m_cols;
  m_cols = other.m_cols;
  other.m_cols = tmp;

  m_data.swap(other.m_data);
}

template <typename T>
void swap(const MatT<T>& o1, const MatT<T>& o2) {
  o1.swap(o2);
}

template <typename T>
MatT<T> operator*(const MatT<T>& mat, double scalar) {
  MatT<T> res(mat);
  res *= scalar;

  return res;
}

template <typename T>
MatT<T> operator*(double scalar, const MatT<T>& mat) {
  MatT<T> res(mat);
  res *= scalar;

  return res;
}

#endif  // ifndef MAT_T_H_
