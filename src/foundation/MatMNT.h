// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_FOUNDATION_MATMNT_H_
#define SCANTAILOR_FOUNDATION_MATMNT_H_

#include <cstddef>

template <size_t M, size_t N, typename T>
class MatMNT;

using Mat22f = MatMNT<2, 2, float>;
using Mat22d = MatMNT<2, 2, double>;
using Mat33f = MatMNT<3, 3, float>;
using Mat33d = MatMNT<3, 3, double>;
using Mat44f = MatMNT<4, 4, float>;
using Mat44d = MatMNT<4, 4, double>;

/**
 * \brief A matrix with pre-defined dimensions.
 *
 * \note The memory layout is always column-major, as that's what MatrixCalc uses.
 */
template <size_t M, size_t N, typename T>
class MatMNT {
 public:
  using type = T;
  enum { ROWS = static_cast<int>(M), COLS = static_cast<int>(N) };

  /**
   * \brief Initializes matrix elements to T().
   */
  MatMNT();

  /**
   * \brief Construction from an array of elements of possibly different type.
   *
   * Conversion is done by static casts.  Data elements must be in column-major order.
   */
  template <typename OT>
  explicit MatMNT(const OT* data);

  /**
   * \brief Construction from a matrix of same dimensions but another type.
   *
   * Conversion is done by static casts.
   */
  template <typename OT>
  explicit MatMNT(const MatMNT<M, N, OT>& other);

  /**
   * \brief Assignment from a matrix of same dimensions but another type.
   *
   * Conversion is done by static casts.
   */
  template <typename OT>
  MatMNT& operator=(const MatMNT<M, N, OT>& other);

  const T* data() const { return m_data; }

  T* data() { return m_data; }

  const T& operator()(int i, int j) const { return m_data[i + j * M]; }

  T& operator()(int i, int j) { return m_data[i + j * M]; }

 private:
  T m_data[M * N];
};


template <size_t M, size_t N, typename T>
MatMNT<M, N, T>::MatMNT() {
  const size_t len = ROWS * COLS;
  for (size_t i = 0; i < len; ++i) {
    m_data[i] = T();
  }
}

template <size_t M, size_t N, typename T>
template <typename OT>
MatMNT<M, N, T>::MatMNT(const OT* data) {
  const size_t len = ROWS * COLS;
  for (size_t i = 0; i < len; ++i) {
    m_data[i] = static_cast<T>(data[i]);
  }
}

template <size_t M, size_t N, typename T>
template <typename OT>
MatMNT<M, N, T>::MatMNT(const MatMNT<M, N, OT>& other) {
  const OT* data = other.data();
  const size_t len = ROWS * COLS;
  for (size_t i = 0; i < len; ++i) {
    m_data[i] = static_cast<T>(data[i]);
  }
}

#endif  // ifndef SCANTAILOR_FOUNDATION_MATMNT_H_
