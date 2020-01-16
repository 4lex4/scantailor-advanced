// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_FOUNDATION_VALUECONV_H_
#define SCANTAILOR_FOUNDATION_VALUECONV_H_

#include <cmath>

#include "NumericTraits.h"

template <typename ToType>
class StaticCastValueConv {
 public:
  template <typename FromType>
  ToType operator()(FromType val) const {
    return static_cast<ToType>(val);
  }
};


template <typename ToType>
class RoundAndClipValueConv {
 public:
  explicit RoundAndClipValueConv(ToType min = NumericTraits<ToType>::min(), ToType max = NumericTraits<ToType>::max())
      : m_min(min), m_max(max) {}

  template <typename FromType>
  ToType operator()(FromType val) const {
    // To avoid possible "comparing signed to unsigned" warnings,
    // we do the comparison with FromType.  It should be fine, as
    // "Round" in the name of the class assumes it's a floating point type,
    // and therefore should be "wider" than ToType.
    if (val < FromType(m_min)) {
      return m_min;
    } else if (val > FromType(m_max)) {
      return m_max;
    } else {
      return static_cast<ToType>(std::floor(val + 0.5));
    }
  }

 private:
  ToType m_min;
  ToType m_max;
};


#endif  // ifndef SCANTAILOR_FOUNDATION_VALUECONV_H_
