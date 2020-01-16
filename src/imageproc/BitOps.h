// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_IMAGEPROC_BITOPS_H_
#define SCANTAILOR_IMAGEPROC_BITOPS_H_

namespace imageproc {
namespace detail {
extern const unsigned char bitCounts[256];
extern const unsigned char reversedBits[256];

template <typename T, int BytesRemaining>
class NonZeroBits {
 public:
  static unsigned char count(T val) {
    return bitCounts[static_cast<unsigned char>(val)] + NonZeroBits<T, BytesRemaining - 1>::count(val >> 8);
  }
};


template <typename T>
class NonZeroBits<T, 1> {
 public:
  static unsigned char count(T val) { return bitCounts[static_cast<unsigned char>(val)]; }
};


template <typename T, int TotalBytes, int Offset, bool Done = false>
struct ReverseBytes {
  static T result(const T val) {
    const int leftShift = (TotalBytes - Offset - 1) * 8;
    const int rightShift = Offset * 8;

    using Byte = unsigned char;
    const Byte leftByte = static_cast<Byte>(val >> leftShift);
    const Byte rightByte = static_cast<Byte>(val >> rightShift);

    T res(ReverseBytes<T, TotalBytes, Offset + 1, Offset == TotalBytes / 2>::result(val));
    res |= T(reversedBits[leftByte]) << rightShift;
    res |= T(reversedBits[rightByte]) << leftShift;
    return res;
  }
};

template <typename T, int TotalBytes, int Offset>
struct ReverseBytes<T, TotalBytes, Offset, true> {
  static T result(T) { return T(); }
};

template <typename T>
struct ReverseBytes<T, 1, 0, false> {
  static T result(const T val) {
    using Byte = unsigned char;
    return T(reversedBits[static_cast<Byte>(val)]);
  }
};


template <typename T, int STRIPE_LEN, int BITS_DONE = 0, bool HAVE_MORE_BITS = true>
struct StripedMaskMSB1 {
  static const T value
      = (((T(1) << STRIPE_LEN) - 1) << (BITS_DONE + STRIPE_LEN))
        | StripedMaskMSB1<T, STRIPE_LEN, BITS_DONE + STRIPE_LEN * 2, BITS_DONE + STRIPE_LEN * 2 < sizeof(T) * 8>::value;
};

template <typename T, int STRIPE_LEN, int BITS_DONE>
struct StripedMaskMSB1<T, STRIPE_LEN, BITS_DONE, false> {
  static const T value = 0;
};


template <typename T, int STRIPE_LEN, int BITS_DONE = 0, bool HAVE_MORE_BITS = true>
struct StripedMaskLSB1 {
  static const T value
      = (((T(1) << STRIPE_LEN) - 1) << BITS_DONE)
        | StripedMaskLSB1<T, STRIPE_LEN, BITS_DONE + STRIPE_LEN * 2, BITS_DONE + STRIPE_LEN * 2 < sizeof(T) * 8>::value;
};

template <typename T, int STRIPE_LEN, int BITS_DONE>
struct StripedMaskLSB1<T, STRIPE_LEN, BITS_DONE, false> {
  static const T value = 0;
};


template <typename T, int STRIPE_LEN>
struct MostSignificantZeroes {
  static int reduce(T val, int count) {
    if (T tmp = val & StripedMaskMSB1<T, STRIPE_LEN>::value) {
      val = tmp;
      count -= STRIPE_LEN;
    }
    return MostSignificantZeroes<T, STRIPE_LEN / 2>::reduce(val, count);
  }
};

template <typename T>
struct MostSignificantZeroes<T, 0> {
  static int reduce(T val, int count) { return count - 1; }
};


template <typename T, int STRIPE_LEN>
struct LeastSignificantZeroes {
  static int reduce(T val, int count) {
    if (T tmp = val & StripedMaskLSB1<T, STRIPE_LEN>::value) {
      val = tmp;
      count -= STRIPE_LEN;
    }
    return LeastSignificantZeroes<T, STRIPE_LEN / 2>::reduce(val, count);
  }
};

template <typename T>
struct LeastSignificantZeroes<T, 0> {
  static int reduce(T val, int count) { return count - 1; }
};
}  // namespace detail

template <typename T>
int countNonZeroBits(const T val) {
  return detail::NonZeroBits<T, sizeof(T)>::count(val);
}

template <typename T>
T reverseBits(const T val) {
  return detail::ReverseBytes<T, sizeof(T), 0>::result(val);
}

template <typename T>
int countMostSignificantZeroes(const T val) {
  static const int totalBits = sizeof(T) * 8;
  int zeroes = totalBits;

  if (val) {
    zeroes = detail::MostSignificantZeroes<T, totalBits / 2>::reduce(val, zeroes);
  }
  return zeroes;
}

template <typename T>
int countLeastSignificantZeroes(const T val) {
  static const int totalBits = sizeof(T) * 8;
  int zeroes = totalBits;

  if (val) {
    zeroes = detail::LeastSignificantZeroes<T, totalBits / 2>::reduce(val, zeroes);
  }
  return zeroes;
}
}  // namespace imageproc
#endif  // ifndef SCANTAILOR_IMAGEPROC_BITOPS_H_
