// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_MATH_MATRIXCALC_H_
#define SCANTAILOR_MATH_MATRIXCALC_H_

#include <cassert>
#include <cstddef>

#include "DynamicPool.h"
#include "LinearSolver.h"
#include "MatMNT.h"
#include "MatT.h"
#include "NonCopyable.h"
#include "StaticPool.h"
#include "VecNT.h"
#include "VecT.h"

template <typename T, typename Alloc>
class MatrixCalc;

namespace mcalc {
template <typename T>
class AbstractAllocator {
 public:
  virtual T* allocT(size_t size) = 0;

  virtual size_t* allocP(size_t size) = 0;
};


template <typename T, size_t TSize, size_t PSize>
class StaticPoolAllocator : public AbstractAllocator<T> {
 public:
  virtual T* allocT(size_t size) { return m_poolT.alloc(size); }

  virtual size_t* allocP(size_t size) { return m_poolP.alloc(size); }

 private:
  StaticPool<size_t, PSize> m_poolP;
  StaticPool<T, TSize> m_poolT;
};


template <typename T>
class DynamicPoolAllocator : public AbstractAllocator<T> {
 public:
  virtual T* allocT(size_t size) { return m_poolT.alloc(size); }

  virtual size_t* allocP(size_t size) { return m_poolP.alloc(size); }

 private:
  DynamicPool<size_t> m_poolP;
  DynamicPool<T> m_poolT;
};


template <typename T>
class Mat {
  template <typename OT, typename Alloc>
  friend class ::MatrixCalc;

  template <typename OT>
  friend Mat<OT> operator+(const Mat<OT>& m1, const Mat<OT>& m2);

  template <typename OT>
  friend Mat<OT> operator-(const Mat<OT>& m1, const Mat<OT>& m2);

  template <typename OT>
  friend Mat<OT> operator*(const Mat<OT>& m1, const Mat<OT>& m2);

  template <typename OT>
  friend Mat<OT> operator*(OT scalar, const Mat<OT>& m);

  template <typename OT>
  friend Mat<OT> operator*(const Mat<OT>& m, OT scalar);

  template <typename OT>
  friend Mat<OT> operator/(const Mat<OT>& m, OT scalar);

 public:
  Mat inv() const;

  Mat solve(const Mat& b) const;

  Mat solve(const T* data, int rows, int cols) const;

  Mat trans() const;

  Mat write(T* buf) const;

  template <size_t N>
  Mat write(VecNT<N, T>& vec) const;

  Mat transWrite(T* buf) const;

  template <size_t N>
  Mat transWrite(VecNT<N, T>& vec) const;

  Mat operator-() const;

  const T* rawData() const { return data; }

 private:
  Mat(AbstractAllocator<T>* alloc, const T* data, int rows, int cols)
      : alloc(alloc), data(data), rows(rows), cols(cols) {}

  AbstractAllocator<T>* alloc;
  const T* data;
  int rows;
  int cols;
};
}  // namespace mcalc

template <typename T, typename Alloc = mcalc::StaticPoolAllocator<T, 128, 9>>
class MatrixCalc {
  DECLARE_NON_COPYABLE(MatrixCalc)

 public:
  MatrixCalc() {}

  mcalc::Mat<T> operator()(const T* data, int rows, int cols) { return mcalc::Mat<T>(&m_alloc, data, rows, cols); }

  template <size_t N>
  mcalc::Mat<T> operator()(const VecNT<N, T>& vec, int rows, int cols) {
    return mcalc::Mat<T>(&m_alloc, vec.data(), rows, cols);
  }

  template <size_t M, size_t N>
  mcalc::Mat<T> operator()(const MatMNT<M, N, T>& mat) {
    return mcalc::Mat<T>(&m_alloc, mat.data(), mat.ROWS, mat.COLS);
  }

  mcalc::Mat<T> operator()(const MatT<T>& mat) {
    return mcalc::Mat<T>(&m_alloc, mat.data(), static_cast<int>(mat.rows()), static_cast<int>(mat.cols()));
  }

  template <size_t N>
  mcalc::Mat<T> operator()(const VecNT<N, T>& vec) {
    return mcalc::Mat<T>(&m_alloc, vec.data(), vec.SIZE, 1);
  }

  mcalc::Mat<T> operator()(const VecT<T>& vec) {
    return mcalc::Mat<T>(&m_alloc, vec.data(), static_cast<int>(vec.size()), 1);
  }

 private:
  Alloc m_alloc;
};


template <typename T, size_t TSize = 128, size_t PSize = 9>
class StaticMatrixCalc : public MatrixCalc<T, mcalc::StaticPoolAllocator<T, TSize, PSize>> {};


template <typename T>
class DynamicMatrixCalc : public MatrixCalc<T, mcalc::DynamicPoolAllocator<T>> {};


/*========================== Implementation =============================*/

namespace mcalc {
template <typename T>
Mat<T> Mat<T>::inv() const {
  assert(cols == rows);

  T* identData = alloc->allocT(rows * cols);
  Mat ident(alloc, identData, rows, cols);
  const int todo = rows * cols;
  for (int i = 0; i < todo; ++i) {
    identData[i] = T();
  }
  for (int i = 0; i < todo; i += rows + 1) {
    identData[i] = T(1);
  }
  return solve(ident);
}

template <typename T>
Mat<T> Mat<T>::solve(const Mat& b) const {
  assert(rows == b.rows);

  T* xData = alloc->allocT(cols * b.cols);
  T* tbuffer = alloc->allocT(cols * (b.rows + b.cols));
  size_t* pbuffer = alloc->allocP(rows);
  LinearSolver(rows, cols, b.cols).solve(data, xData, b.data, tbuffer, pbuffer);
  return Mat(alloc, xData, cols, b.cols);
}

template <typename T>
Mat<T> Mat<T>::solve(const T* data, int rows, int cols) const {
  return solve(Mat(alloc, data, rows, cols));
}

template <typename T>
Mat<T> Mat<T>::trans() const {
  if ((cols == 1) || (rows == 1)) {
    return Mat<T>(alloc, data, cols, rows);
  }

  T* pTrans = alloc->allocT(cols * rows);
  transWrite(pTrans);
  return Mat(alloc, pTrans, cols, rows);
}

template <typename T>
Mat<T> Mat<T>::write(T* buf) const {
  const int todo = rows * cols;
  for (int i = 0; i < todo; ++i) {
    buf[i] = data[i];
  }
  return *this;
}

template <typename T>
template <size_t N>
Mat<T> Mat<T>::write(VecNT<N, T>& vec) const {
  assert(N >= size_t(rows * cols));
  return write(vec.data());
}

template <typename T>
Mat<T> Mat<T>::transWrite(T* buf) const {
  T* pTrans = buf;
  for (int i = 0; i < rows; ++i) {
    const T* pSrc = data + i;
    for (int j = 0; j < cols; ++j) {
      *pTrans = *pSrc;
      ++pTrans;
      pSrc += rows;
    }
  }
  return *this;
}

template <typename T>
template <size_t N>
Mat<T> Mat<T>::transWrite(VecNT<N, T>& vec) const {
  assert(N >= rows * cols);
  return transWrite(vec.data());
}

/** Unary minus. */
template <typename T>
Mat<T> Mat<T>::operator-() const {
  T* pRes = alloc->allocT(rows * cols);
  Mat<T> res(alloc, pRes, rows, cols);

  const int todo = rows * cols;
  for (int i = 0; i < todo; ++i) {
    pRes[i] = -data[i];
  }
  return res;
}

template <typename T>
Mat<T> operator+(const Mat<T>& m1, const Mat<T>& m2) {
  assert(m1.rows == m2.rows && m1.cols == m2.cols);

  T* pRes = m1.alloc->allocT(m1.rows * m1.cols);
  Mat<T> res(m1.alloc, pRes, m1.rows, m1.cols);

  const int todo = m1.rows * m1.cols;
  for (int i = 0; i < todo; ++i) {
    pRes[i] = m1.data[i] + m2.data[i];
  }
  return res;
}

template <typename T>
Mat<T> operator-(const Mat<T>& m1, const Mat<T>& m2) {
  assert(m1.rows == m2.rows && m1.cols == m2.cols);

  T* pRes = m1.alloc->allocT(m1.rows * m1.cols);
  Mat<T> res(m1.alloc, pRes, m1.rows, m1.cols);

  const int todo = m1.rows * m1.cols;
  for (int i = 0; i < todo; ++i) {
    pRes[i] = m1.data[i] - m2.data[i];
  }
  return res;
}

template <typename T>
Mat<T> operator*(const Mat<T>& m1, const Mat<T>& m2) {
  assert(m1.cols == m2.rows);

  T* pRes = m1.alloc->allocT(m1.rows * m2.cols);
  Mat<T> res(m1.alloc, pRes, m1.rows, m2.cols);

  for (int rcol = 0; rcol < res.cols; ++rcol) {
    for (int rrow = 0; rrow < res.rows; ++rrow) {
      const T* pM1 = m1.data + rrow;
      const T* pM2 = m2.data + rcol * m2.rows;
      T sum = T();
      for (int i = 0; i < m1.cols; ++i) {
        sum += *pM1 * *pM2;
        pM1 += m1.rows;
        ++pM2;
      }
      *pRes = sum;
      ++pRes;
    }
  }
  return res;
}

template <typename T>
Mat<T> operator*(T scalar, const Mat<T>& m) {
  T* pRes = m.alloc->allocT(m.rows * m.cols);
  Mat<T> res(m.alloc, pRes, m.rows, m.cols);

  const int todo = m.rows * m.cols;
  for (int i = 0; i < todo; ++i) {
    pRes[i] = m.data[i] * scalar;
  }
  return res;
}

template <typename T>
Mat<T> operator*(const Mat<T>& m, T scalar) {
  return scalar * m;
}

template <typename T>
Mat<T> operator/(const Mat<T>& m, T scalar) {
  return m * (1.0f / scalar);
}
}  // namespace mcalc
#endif  // ifndef SCANTAILOR_MATH_MATRIXCALC_H_
