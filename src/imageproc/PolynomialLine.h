// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_IMAGEPROC_POLYNOMIALLINE_H_
#define SCANTAILOR_IMAGEPROC_POLYNOMIALLINE_H_

#include <cmath>
#include <limits>
#include "VecT.h"

namespace imageproc {
/**
 * \brief A polynomial function describing a sequence of numbers.
 */
class PolynomialLine {
  // Member-wise copying is OK.
 public:
  /**
   * \brief Calculate a polynomial that approximates a sequence of values.
   *
   * \param degree The degree of a polynomial to be constructed.
   *        If there are too few data points, the degree may
   *        be silently reduced.  The minimum degree is 0.
   * \param values The data points to be approximated.
   * \param numValues The number of data points to be approximated.
   *        There has to be at least one data point.
   * \param step The distance between adjacent data points.
   *        The data points will be accessed like this:\n
   *        values[0], values[step], values[step * 2]
   */
  template <typename T>
  PolynomialLine(int degree, const T* values, int numValues, int step);

  /**
   * \brief Output the polynomial as a sequence of values.
   *
   * \param values The data points to be written.  If T is
   *        an integer, the values will be rounded and clipped
   *        to the minimum and maximum values for type T,
   *        which are taken from std::numeric_limits<T>.
   *        Otherwise, a static cast will be used to covert
   *        values from double to type T.  If you need
   *        some other behaviour, use the overloaded version
   *        of this method and supply your own post-processor.
   * \param numValues The number of data points to write.  If this
   *        number is different from the one that was used to
   *        construct a polynomial, the output will be scaled
   *        to fit the new size.
   * \param step The distance between adjacent data points.
   *        The data points will be accessed like this:\n
   *        values[0], values[step], values[step * 2]
   */
  template <typename T>
  void output(T* values, int numValues, int step) const;

  /**
   * \brief Output the polynomial as a sequence of values.
   *
   * \param values The data points to be written.
   * \param numValues The number of data points to write.  If this
   *        number is different from the one that was used to
   *        construct a polynomial, the output will be scaled
   *        to fit the new size.
   * \param step The distance between adjacent data points.
   *        The data points will be accessed like this:\n
   *        values[0], values[step], values[step * 2]
   * \param pp A functor to convert a double value to type T.
   *        The functor will be called like this:\n
   *        T t = pp((double)val);
   */
  template <typename T, typename PostProcessor>
  void output(T* values, int numValues, int step, PostProcessor pp) const;

 private:
  template <typename T>
  class StaticCastPostProcessor {
   public:
    T operator()(double val) const;
  };


  template <typename T>
  class RoundAndClipPostProcessor {
   public:
    RoundAndClipPostProcessor();

    T operator()(double val) const;

   private:
    T m_min;
    T m_max;
  };


  template <typename T, bool IsInteger>
  struct DefaultPostProcessor : public StaticCastPostProcessor<T> {};

  template <typename T>
  struct DefaultPostProcessor<T, true> : public RoundAndClipPostProcessor<T> {};

  static void validateArguments(int degree, int numValues);

  static double calcScale(int numValues);

  static void doLeastSquares(const VecT<double>& dataPoints, VecT<double>& coeffs);

  VecT<double> m_coeffs;
};


template <typename T>
inline T PolynomialLine::StaticCastPostProcessor<T>::operator()(const double val) const {
  return static_cast<T>(val);
}

template <typename T>
PolynomialLine::RoundAndClipPostProcessor<T>::RoundAndClipPostProcessor()
    : m_min(std::numeric_limits<T>::min()), m_max(std::numeric_limits<T>::max()) {}

template <typename T>
inline T PolynomialLine::RoundAndClipPostProcessor<T>::operator()(const double val) const {
  const double rounded = std::floor(val + 0.5);
  if (rounded < m_min) {
    return m_min;
  } else if (rounded > m_max) {
    return m_max;
  } else {
    return static_cast<T>(rounded);
  }
}

template <typename T>
PolynomialLine::PolynomialLine(int degree, const T* values, const int numValues, const int step) {
  validateArguments(degree, numValues);

  if (degree + 1 > numValues) {
    degree = numValues - 1;
  }

  const int numTerms = degree + 1;

  VecT<double> dataPoints(numValues);
  for (int i = 0; i < numValues; ++i, values += step) {
    dataPoints[i] = *values;
  }

  VecT<double>(numTerms).swap(m_coeffs);
  doLeastSquares(dataPoints, m_coeffs);
}

template <typename T>
void PolynomialLine::output(T* values, int numValues, int step) const {
  using PP = DefaultPostProcessor<T, std::numeric_limits<T>::is_integer>;
  output(values, numValues, step, PP());
}

template <typename T, typename PostProcessor>
void PolynomialLine::output(T* values, int numValues, int step, PostProcessor pp) const {
  if (numValues <= 0) {
    return;
  }

  // Pretend that data points are positioned in range of [0, 1].
  const double scale = calcScale(numValues);
  for (int i = 0; i < numValues; ++i, values += step) {
    const double position = i * scale;
    double sum = 0.0;
    double pow = 1.0;

    const double* pCoeffs = m_coeffs.data();
    const double* const pCoeffsEnd = pCoeffs + m_coeffs.size();
    for (; pCoeffs != pCoeffsEnd; ++pCoeffs, pow *= position) {
      sum += *pCoeffs * pow;
    }

    *values = pp(sum);
  }
}
}  // namespace imageproc

#endif  // ifndef SCANTAILOR_IMAGEPROC_POLYNOMIALLINE_H_
