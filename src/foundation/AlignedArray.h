// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_FOUNDATION_ALIGNEDARRAY_H_
#define SCANTAILOR_FOUNDATION_ALIGNEDARRAY_H_

#include <cstddef>
#include <cstdint>
#include "NonCopyable.h"

/**
 * \brief An array of elements starting at address with a specified alignment.
 *
 * The alignment is specified not in terms of bytes, but in terms of units,
 * where bytes = units * sizeof(T)
 */
template <typename T, size_t alignment_in_units>
class AlignedArray {
  DECLARE_NON_COPYABLE(AlignedArray)

 public:
  /**
   * \brief Constructs a null array.
   */
  AlignedArray() : m_alignedData(0), m_storage(0) {}

  explicit AlignedArray(size_t size);

  ~AlignedArray() { delete[] m_storage; }

  T* data() { return m_alignedData; }

  const T* data() const { return m_alignedData; }

  T& operator[](size_t idx) { return m_alignedData[idx]; }

  const T& operator[](size_t idx) const { return m_alignedData[idx]; }

  void swap(AlignedArray& other);

 private:
  T* m_alignedData;
  T* m_storage;
};


template <typename T, size_t alignment_in_units>
inline void swap(AlignedArray<T, alignment_in_units>& o1, AlignedArray<T, alignment_in_units>& o2) {
  o1.swap(o2);
}

template <typename T, size_t alignment_in_units>
AlignedArray<T, alignment_in_units>::AlignedArray(size_t size) {
  const int a = static_cast<int>(alignment_in_units > 1 ? alignment_in_units : 1);
  const int am1 = a - 1;
  m_storage = new T[size + am1];
  m_alignedData = m_storage + ((a - ((uintptr_t(m_storage) / sizeof(T)) & am1)) & am1);
}

template <typename T, size_t alignment_in_units>
void AlignedArray<T, alignment_in_units>::swap(AlignedArray& other) {
  T* temp = m_alignedData;
  m_alignedData = other.m_alignedData;
  other.m_alignedData = temp;

  temp = m_storage;
  m_storage = other.m_storage;
  other.m_storage = temp;
}

#endif  // ifndef SCANTAILOR_FOUNDATION_ALIGNEDARRAY_H_
