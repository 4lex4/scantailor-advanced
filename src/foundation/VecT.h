// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_FOUNDATION_VECT_H_
#define SCANTAILOR_FOUNDATION_VECT_H_

#include <boost/scoped_array.hpp>
#include <cassert>
#include <cstddef>

/**
 * \brief A (column) vector of elements of type T.
 */
template <typename T>
class VecT {
 public:
  using type = T;

  /**
   * \brief Constructs an empty vector.
   */
  VecT();

  /**
   * \brief Constructs a vector of specified size initialized with T().
   */
  explicit VecT(size_t size);

  /**
   * \brief Constructs a vector of specified size initializing to the provided value.
   */
  VecT(size_t size, T initialValue);

  /**
   * \brief Construction from an array of elements of possibly different type.
   *
   * Conversion is done by static casts.
   */
  template <typename OT>
  explicit VecT(size_t size, const OT* data);

  /**
   * Ordinary copy-construction.
   */
  VecT(const VecT& other);

  /**
   * \brief Construction from a vector of a different type.
   *
   * Conversion is done by static casts.
   */
  template <typename OT>
  explicit VecT(const VecT<OT>& other);

  /**
   * \brief Ordinary assignment.
   */
  VecT& operator=(const VecT& other);

  /**
   * \brief Assignment from a vector of a different type.
   *
   * Conversion is done by static casts.
   */
  template <typename OT>
  VecT& operator=(const VecT<OT>& other);

  VecT& operator+=(const VecT& rhs);

  VecT& operator-=(const VecT& rhs);

  VecT& operator*=(T scalar);

  size_t size() const { return m_size; }

  const T* data() const { return m_data.get(); }

  T* data() { return m_data.get(); }

  const T& operator[](size_t idx) const {
    assert(idx < m_size);
    return m_data[idx];
  }

  T& operator[](size_t idx) {
    assert(idx < m_size);
    return m_data[idx];
  }

  void fill(const T& value);

  void swap(VecT& other);

 private:
  boost::scoped_array<T> m_data;
  size_t m_size;
};


template <typename T>
VecT<T>::VecT() : m_size(0) {}

template <typename T>
VecT<T>::VecT(size_t size)
    : m_data(new T[size]()),
      // The "()" will cause elements to be initialized to T().
      m_size(size) {}

template <typename T>
VecT<T>::VecT(size_t size, T initialValue) : m_data(new T[size]), m_size(size) {
  for (size_t i = 0; i < size; ++i) {
    m_data[i] = initialValue;
  }
}

template <typename T>
template <typename OT>
VecT<T>::VecT(size_t size, const OT* data) : m_data(new T[size]), m_size(size) {
  for (size_t i = 0; i < size; ++i) {
    m_data[i] = static_cast<T>(data[i]);
  }
}

template <typename T>
VecT<T>::VecT(const VecT& other) : m_data(new T[other.m_size]), m_size(other.m_size) {
  const T* otherData = other.data();
  for (size_t i = 0; i < m_size; ++i) {
    m_data[i] = otherData[i];
  }
}

template <typename T>
template <typename OT>
VecT<T>::VecT(const VecT<OT>& other) : m_data(new T[other.m_size]), m_size(other.m_size) {
  const T* otherData = other.data();
  for (size_t i = 0; i < m_size; ++i) {
    m_data[i] = otherData[i];
  }
}

template <typename T>
VecT<T>& VecT<T>::operator=(const VecT& other) {
  VecT(other).swap(*this);
  return *this;
}

template <typename T>
template <typename OT>
VecT<T>& VecT<T>::operator=(const VecT<OT>& other) {
  VecT(other).swap(*this);
  return *this;
}

template <typename T>
VecT<T>& VecT<T>::operator+=(const VecT& rhs) {
  assert(m_size == rhs.m_size);
  for (size_t i = 0; i < m_size; ++i) {
    m_data[i] += rhs.m_data[i];
  }
  return *this;
}

template <typename T>
VecT<T>& VecT<T>::operator-=(const VecT& rhs) {
  assert(m_size == rhs.m_size);
  for (size_t i = 0; i < m_size; ++i) {
    m_data[i] -= rhs.m_data[i];
  }
  return *this;
}

template <typename T>
VecT<T>& VecT<T>::operator*=(const T scalar) {
  for (size_t i = 0; i < m_size; ++i) {
    m_data[i] *= scalar;
  }
  return *this;
}

template <typename T>
void VecT<T>::fill(const T& value) {
  for (size_t i = 0; i < m_size; ++i) {
    m_data[i] = value;
  }
}

template <typename T>
void VecT<T>::swap(VecT& other) {
  size_t tmp = m_size;
  m_size = other.m_size;
  other.m_size = tmp;
  m_data.swap(other.m_data);
}

template <typename T>
void swap(const VecT<T>& o1, const VecT<T>& o2) {
  o1.swap(o2);
}

template <typename T>
VecT<T> operator*(const VecT<T>& vec, double scalar) {
  VecT<T> res(vec);
  res *= scalar;
  return res;
}

template <typename T>
VecT<T> operator*(double scalar, const VecT<T>& vec) {
  VecT<T> res(vec);
  res *= scalar;
  return res;
}

#endif  // ifndef SCANTAILOR_FOUNDATION_VECT_H_
