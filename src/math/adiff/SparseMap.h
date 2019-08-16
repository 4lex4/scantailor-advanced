// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef ADIFF_SPARSITY_H_
#define ADIFF_SPARSITY_H_

#include <cstddef>
#include "MatT.h"

namespace adiff {
/**
 * Specifies which derivatives are non-zero and therefore need to be calculated.
 * Each such non-zero derivative is assigned an index in [0, total_nonzero_derivs).
 */
template <int ORD>
class SparseMap;

/**
 * The second order sparse map specified which elements of the Hessian
 * matrix are non-zero.
 */
template <>
class SparseMap<2> {
  // Member-wise copying is OK.
 public:
  static const size_t ZERO_ELEMENT;

  /**
   * Creates a structure for a (num_vars)x(num_vars) Hessian
   * with all elements being initially considered as zero.
   */
  explicit SparseMap(size_t num_vars);

  /**
   * Returns N for an NxN Hessian.
   */
  size_t numVars() const { return m_numVars; }

  /**
   * \brief Marks an element at (i, j) as non-zero.
   *
   * Calling this function on an element already marked non-zero
   * has no effect.
   */
  void markNonZero(size_t i, size_t j);

  /**
   * \brief Marks all elements as non-zero.
   *
   * The caller shouldn't assume any particular pattern of index
   * assignment when using this function.
   */
  void markAllNonZero();

  /**
   * Returns the number of elements marked as non-zero.
   */
  size_t numNonZeroElements() const { return m_numNonZeroElements; }

  /**
   * Returns an index in the range of [0, numNonZeroElements)
   * associated with the element, or ZERO_ELEMENT, if the element
   * wasn't marked non-zero.
   */
  size_t nonZeroElementIdx(size_t i, size_t j) const;

 private:
  size_t m_numVars;
  size_t m_numNonZeroElements;
  MatT<size_t> m_map;
};
}  // namespace adiff
#endif  // ifndef ADIFF_SPARSITY_H_
