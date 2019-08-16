// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef NUMERIC_TRAITS_H_
#define NUMERIC_TRAITS_H_

#include <limits>

namespace numeric_traits_impl {
template <typename T, bool IsInteger>
struct IntegerSpecific;
}  // namespace numeric_traits_impl

/**
 * This class exists mainly because std::numeric_values<>::min() has
 * inconsistent behaviour for integer vs floating point types.
 */
template <typename T>
class NumericTraits {
 public:
  static T max() { return std::numeric_limits<T>::max(); }

  /**
   * This one behaves as you expect, not as std::numeric_limits<T>::min().
   * That is, this one will actually give you a negative value both for
   * integer and floating point types.
   */
  static T min() { return numeric_traits_impl::IntegerSpecific<T, std::numeric_limits<T>::is_integer>::min(); }

 private:
};


namespace numeric_traits_impl {
template <typename T>
struct IntegerSpecific<T, true> {
  static T min() { return std::numeric_limits<T>::min(); }
};

template <typename T>
struct IntegerSpecific<T, false> {
  static T min() { return -std::numeric_limits<T>::max(); }
};
}  // namespace numeric_traits_impl
#endif  // ifndef NUMERIC_TRAITS_H_
