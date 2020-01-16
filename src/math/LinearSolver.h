// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_MATH_LINEARSOLVER_H_
#define SCANTAILOR_MATH_LINEARSOLVER_H_

#include <NonCopyable.h>

#include <boost/scoped_array.hpp>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <limits>
#include <stdexcept>
#include <string>

#include "StaticPool.h"

/**
 * \brief Solves Ax = b using LU decomposition.
 *
 * Overdetermined systems are supported.  Solving them will succeed
 * provided the system is consistent.
 *
 * \note All matrices are assumed to be in column-major order.
 *
 * \see MatrixCalc
 */
class LinearSolver {
  // Member-wise copying is OK.
 public:
  /*
   * \throw std::runtime_error If rows_AB < cols_A_rows_X.
   */
  LinearSolver(size_t rows_AB, size_t cols_A_rows_X, size_t cols_BX);

  /**
   * \brief Solves Ax = b
   *
   * \param A Matrix A.
   * \param X Matrix (or vector) X.  Results will be written here.
   * \param B Matrix (or vector) B.  It's allowed to pass the same pointer for X and B.
   * \param tbuffer Temporary buffer of at least "cols(A) * (rows(B) + cols(B))" T elements.
   * \param pbuffer Temporary buffer of at least "rows(B)" size_t elements.
   *
   * \throw std::runtime_error If the system can't be solved.
   */
  template <typename T>
  void solve(const T* A, T* X, const T* B, T* tbuffer, size_t* pbuffer) const;

  /**
   * \brief A simplified version of the one above.
   *
   * In this version, buffers are allocated internally.
   */
  template <typename T>
  void solve(const T* A, T* X, const T* B) const;

 private:
  size_t m_rowsAB;
  size_t m_colsArowsX;
  size_t m_colsBX;
};


template <typename T>
void LinearSolver::solve(const T* A, T* X, const T* B, T* tbuffer, size_t* pbuffer) const {
  using namespace std;  // To catch different overloads of std::abs()
  const T epsilon(std::sqrt(numeric_limits<T>::epsilon()));

  const size_t num_elements_A = m_rowsAB * m_colsArowsX;

  T* const luData = tbuffer;  // Dimensions: m_rowsAB, m_colsArowsX
  tbuffer += num_elements_A;

  // Copy this matrix to lu.
  for (size_t i = 0; i < num_elements_A; ++i) {
    luData[i] = A[i];
  }

  // Maps virtual row numbers to physical ones.
  size_t* const perm = pbuffer;
  for (size_t i = 0; i < m_rowsAB; ++i) {
    perm[i] = i;
  }

  T* pCol = luData;
  for (size_t i = 0; i < m_colsArowsX; ++i, pCol += m_rowsAB) {
    // Find the largest pivot.
    size_t virtPivotRow = i;
    T largestAbsPivot(std::abs(pCol[perm[i]]));
    for (size_t j = i + 1; j < m_rowsAB; ++j) {
      const T absPivot(std::abs(pCol[perm[j]]));
      if (absPivot > largestAbsPivot) {
        largestAbsPivot = absPivot;
        virtPivotRow = j;
      }
    }

    if (largestAbsPivot <= epsilon) {
      throw std::runtime_error("LinearSolver: not a full rank matrix");
    }

    const size_t physPivotRow(perm[virtPivotRow]);
    perm[virtPivotRow] = perm[i];
    perm[i] = physPivotRow;

    const T* const pPivot = pCol + physPivotRow;
    const T rPivot(T(1) / *pPivot);

    // Eliminate entries below the pivot.
    for (size_t j = i + 1; j < m_rowsAB; ++j) {
      const T* p1 = pPivot;
      T* p2 = pCol + perm[j];
      if (std::abs(*p2) <= epsilon) {
        // We consider it's already zero.
        *p2 = T();
        continue;
      }

      const T factor(*p2 * rPivot);
      *p2 = factor;  // Factor goes into L, zero goes into U.
      // Transform the rest of the row.
      for (size_t col = i + 1; col < m_colsArowsX; ++col) {
        p1 += m_rowsAB;
        p2 += m_rowsAB;
        *p2 -= *p1 * factor;
      }
    }
  }

  // First solve Ly = b
  T* const yData = tbuffer;  // Dimensions: m_colsArowsX, m_colsBX
  // tbuffer += m_colsArowsX * m_colsBX;
  T* pYCol = yData;
  const T* pBCol = B;
  for (size_t yCol = 0; yCol < m_colsBX; ++yCol) {
    size_t virtRow = 0;
    for (; virtRow < m_colsArowsX; ++virtRow) {
      const int physRow = static_cast<int>(perm[virtRow]);
      T right(pBCol[physRow]);

      // Move already calculated factors to the right side.
      const T* pLu = luData + physRow;
      // Go left to right, stop at diagonal.
      for (size_t luCol = 0; luCol < virtRow; ++luCol) {
        right -= *pLu * pYCol[luCol];
        pLu += m_rowsAB;
      }

      // We assume L has ones on the diagonal, so no division here.
      pYCol[virtRow] = right;
    }

    // Continue below the square part (if any).
    for (; virtRow < m_rowsAB; ++virtRow) {
      const int physRow = static_cast<int>(perm[virtRow]);
      T right(pBCol[physRow]);

      // Move everything to the right side, then verify it's zero.
      const T* pLu = luData + physRow;
      // Go left to right all the way.
      for (size_t luCol = 0; luCol < m_colsArowsX; ++luCol) {
        right -= *pLu * pYCol[luCol];
        pLu += m_rowsAB;
      }
      if (std::abs(right) > epsilon) {
        throw std::runtime_error("LinearSolver: inconsistent overdetermined system");
      }
    }

    pYCol += m_colsArowsX;
    pBCol += m_rowsAB;
  }

  // Now solve Ux = y
  T* pXCol = X;
  pYCol = yData;
  const T* pLuLastCol = luData + (m_colsArowsX - 1) * m_rowsAB;
  for (size_t xCol = 0; xCol < m_colsBX; ++xCol) {
    for (int virtRow = static_cast<int>(m_colsArowsX - 1); virtRow >= 0; --virtRow) {
      T right(pYCol[virtRow]);

      // Move already calculated factors to the right side.
      const T* pLu = pLuLastCol + perm[virtRow];
      // Go right to left, stop at diagonal.
      for (int luCol = static_cast<int>(m_colsArowsX - 1); luCol > virtRow; --luCol) {
        right -= *pLu * pXCol[luCol];
        pLu -= m_rowsAB;
      }
      pXCol[virtRow] = right / *pLu;
    }

    pXCol += m_colsArowsX;
    pYCol += m_colsArowsX;
  }
}  // LinearSolver::solve

template <typename T>
void LinearSolver::solve(const T* A, T* X, const T* B) const {
  boost::scoped_array<T> tbuffer(new T[m_colsArowsX * (m_rowsAB + m_colsBX)]);
  boost::scoped_array<size_t> pbuffer(new size_t[m_rowsAB]);

  solve(A, X, B, tbuffer.get(), pbuffer.get());
}

#endif  // ifndef SCANTAILOR_MATH_LINEARSOLVER_H_
