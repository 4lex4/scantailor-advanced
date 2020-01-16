// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_IMAGEPROC_RASTEROPGENERIC_H_
#define SCANTAILOR_IMAGEPROC_RASTEROPGENERIC_H_

#include <QSize>
#include <cassert>
#include <cstdint>

#include "BinaryImage.h"

namespace imageproc {
/**
 * \brief Perform an operation on a single image.
 *
 * \param data The pointer to image data.
 * \param stride The number of elements of type T per image line.
 * \param size Image size.
 * \param operation An operation to perform.  It will be called like this:
 * \code
 * operation(data[offset]);
 * \endcode
 * Depending on whether T is const, the operation may be able to modify the image.
 * Hinst: boost::lambda is an easy way to construct operations.
 */
template <typename T, typename Op>
void rasterOpGeneric(T* data, int stride, QSize size, Op operation);

/**
 * \brief Perform an operation on a pair of images.
 *
 * \param data1 The pointer to image data of the first image.
 * \param stride1 The number of elements of type T1 per line of the first image.
 * \param size Dimensions of both images.
 * \param data2 The pointer to image data of the second image.
 * \param stride2 The number of elements of type T2 per line of the second image.
 * \param operation An operation to perform.  It will be called like this:
 * \code
 * operation(data1[offset1], data2[offset2]);
 * \endcode
 * Depending on whether T1 / T2 are const, the operation may be able to modify
 * one or both of them.
 * Hinst: boost::lambda is an easy way to construct operations.
 */
template <typename T1, typename T2, typename Op>
void rasterOpGeneric(T1* data1, int stride1, QSize size, T2* data2, int stride2, Op operation);


/**
 * \brief Same as the one above, except one of the images is a const BinaryImage.
 *
 * \p operation will be called like this:
 * \code
 * const uint32_t bit1 = <something> ? 1 : 0;
 * operation(bitl, data2[offset2]);
 * \endcode
 */
template <typename T2, typename Op>
void rasterOpGeneric(const BinaryImage& image1, T2* data2, int stride2, Op operation);

/**
 * \brief Same as the one above, except one of the images is a non-const BinaryImage.
 *
 * \p operation will be called like this:
 * \code
 * BitProxy bit1(...);
 * operation(bitl, data2[offset2]);
 * \endcode
 * BitProxy will have implicit conversion to uint32_t returning 0 or 1,
 * and an assignment operator from uint32_t, expecting 0 or 1 only.
 */
template <typename T2, typename Op>
void rasterOpGeneric(const BinaryImage& image1, T2* data2, int stride2, Op operation);


/*======================== Implementation ==========================*/

template <typename T, typename Op>
void rasterOpGeneric(T* data, int stride, QSize size, Op operation) {
  if (size.isEmpty()) {
    return;
  }

  const int w = size.width();
  const int h = size.height();

  for (int y = 0; y < h; ++y) {
    for (int x = 0; x < w; ++x) {
      operation(data[x]);
    }
    data += stride;
  }
}

template <typename T1, typename T2, typename Op>
void rasterOpGeneric(T1* data1, int stride1, QSize size, T2* data2, int stride2, Op operation) {
  if (size.isEmpty()) {
    return;
  }

  const int w = size.width();
  const int h = size.height();

  for (int y = 0; y < h; ++y) {
    for (int x = 0; x < w; ++x) {
      operation(data1[x], data2[x]);
    }
    data1 += stride1;
    data2 += stride2;
  }
}

template <typename T2, typename Op>
void rasterOpGeneric(const BinaryImage& image1, T2* data2, int stride2, Op operation) {
  if (image1.isNull()) {
    return;
  }

  const int w = image1.width();
  const int h = image1.height();
  const int stride1 = image1.wordsPerLine();
  const uint32_t* data1 = image1.data();

  for (int y = 0; y < h; ++y) {
    for (int x = 0; x < w; ++x) {
      const int shift = 31 - (x & 31);
      operation((data1[x >> 5] >> shift) & uint32_t(1), data2[x]);
    }
    data1 += stride1;
    data2 += stride2;
  }
}

namespace rop_generic_impl {
class BitProxy {
 public:
  BitProxy(uint32_t& word, int shift) : m_word(word), m_shift(shift) {}

  BitProxy(const BitProxy& other) = default;

  BitProxy& operator=(uint32_t bit) {
    assert(bit <= 1);
    const uint32_t mask = uint32_t(1) << m_shift;
    m_word = (m_word & ~mask) | (bit << m_shift);
    return *this;
  }

  operator uint32_t() const { return (m_word >> m_shift) & uint32_t(1); }

 private:
  uint32_t& m_word;
  int m_shift;
};
}  // namespace rop_generic_impl

template <typename T2, typename Op>
void rasterOpGeneric(BinaryImage& image1, T2* data2, int stride2, Op operation) {
  using namespace rop_generic_impl;

  if (image1.isNull()) {
    return;
  }

  const int w = image1.width();
  const int h = image1.height();
  const int stride1 = image1.wordsPerLine();
  uint32_t* data1 = image1.data();

  for (int y = 0; y < h; ++y) {
    for (int x = 0; x < w; ++x) {
      BitProxy bit1(data1[x >> 5], 31 - (x & 31));
      operation(bit1, data2[x]);
    }
    data1 += stride1;
    data2 += stride2;
  }
}
}  // namespace imageproc
#endif  // ifndef SCANTAILOR_IMAGEPROC_RASTEROPGENERIC_H_
