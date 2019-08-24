// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_IMAGEPROC_SOBEL_H_
#define SCANTAILOR_IMAGEPROC_SOBEL_H_

/**
 * \file
 *
 * The Sobel operator treats a 2D grid of data points as a function f(x, y)
 * and approximates its gradient.  It computes not the gradient itself,
 * but the gradient multiplied by 8.
 */

namespace imageproc {
/**
 * Computes approximation of the horizontal gradient component, that is
 * the partial derivative with respect to x (multiplied by 8).
 *
 * \tparam T The type used for intermediate calculations.  Must be signed.
 * \param width Horizontal size of a grid.  Zero or negative value
 *        will cause this function to return without doing anything.
 * \param height Vertical size of a grid.  Zero or negative value
 *        will cause this function to return without doing anything.
 * \param src Pointer or a random access iterator to the top-left corner
 *        of the source grid.
 * \param srcStride The distance from a point on the source grid to the
 *        point directly below it, in terms of iterator difference.
 * \param srcReader A functor that gets passed a dereferenced iterator
 *        to the source grid and returns some type convertable to T.
 *        It's called like this:
 *        \code
 *        SrcIt src_it = ...;
 *        const T var(srcReader(*src_it));
 *        \endcode
 *        Consider using boost::lambda for constructing such a functor,
 *        possibly combined with one of the functors from ValueConf.h
 * \param tmp Pointer or a random access iterator to the top-left corner
 *        of the temporary grid.  The temporary grid will have the same
 *        width and height as the source and destination grids.
 *        Having the destination grid also serve as a temporary grid
 *        is supported, provided it's able to store signed values.
 *        Having all 3 to be the same is supported as well, subject
 *        to the same condition.
 * \param tmpStride The distance from a point on the temporary grid to the
 *        point directly below it, in terms of iterator difference.
 * \param tmpWriter A functor that writes a value to the temporary grid.
 *        It's called like this:
 *        \code
 *        TmpIt tmp_it = ...;
 *        T val = ...;
 *        tmpWriter(*tmp_it, val);
 *        \endcode
 * \param tmpReader A functor that gets passed a dereferenced iterator
 *        to the temporary grid and returns some type convertable to T.
 *        See \p srcReader for more info.
 * \param dst Pointer or a random access iterator to the top-left corner
 *        of the destination grid.
 * \param dstStride The distance from a point on the destination grid to the
 *        point directly below it, in terms of iterator difference.
 * \param dstWriter A functor that writes a value to the destination grid.
 *        See \p tmpWriter for more info.
 */
template <typename T,
          typename SrcIt,
          typename TmpIt,
          typename DstIt,
          typename SrcReader,
          typename TmpWriter,
          typename TmpReader,
          typename DstWriter>
void horizontalSobel(int width,
                     int height,
                     SrcIt src,
                     int srcStride,
                     SrcReader srcReader,
                     TmpIt tmp,
                     int tmpStride,
                     TmpWriter tmpWriter,
                     TmpReader tmpReader,
                     DstIt dst,
                     int dstStride,
                     DstWriter dstWriter);


/**
 * \see horizontalSobel()
 */
template <typename T,
          typename SrcIt,
          typename TmpIt,
          typename DstIt,
          typename SrcReader,
          typename TmpWriter,
          typename TmpReader,
          typename DstWriter>
void verticalSobel(int width,
                   int height,
                   SrcIt src,
                   int srcStride,
                   SrcReader srcReader,
                   TmpIt tmp,
                   int tmpStride,
                   TmpWriter tmpWriter,
                   TmpReader tmpReader,
                   DstIt dst,
                   int dstStride,
                   DstWriter dstWriter);


template <typename T,
          typename SrcIt,
          typename TmpIt,
          typename DstIt,
          typename SrcReader,
          typename TmpWriter,
          typename TmpReader,
          typename DstWriter>
void horizontalSobel(const int width,
                     const int height,
                     SrcIt src,
                     int srcStride,
                     SrcReader srcReader,
                     TmpIt tmp,
                     const int tmpStride,
                     TmpWriter tmpWriter,
                     TmpReader tmpReader,
                     DstIt dst,
                     const int dstStride,
                     DstWriter dstWriter) {
  if ((width <= 0) || (height <= 0)) {
    return;
  }

  // Vertical pre-accumulation pass: mid = top + mid*2 + bottom
  for (int x = 0; x < width; ++x) {
    SrcIt pSrc(src + x);
    TmpIt pTmp(tmp + x);

    T top(srcReader(*pSrc));
    if (height == 1) {
      tmpWriter(*pTmp, top + top + top + top);
      continue;
    }

    T mid(srcReader(pSrc[srcStride]));
    tmpWriter(*pTmp, top + top + top + mid);

    for (int y = 1; y < height - 1; ++y) {
      pSrc += srcStride;
      pTmp += tmpStride;
      const T bottom(srcReader(pSrc[srcStride]));
      tmpWriter(*pTmp, top + mid + mid + bottom);
      top = mid;
      mid = bottom;
    }

    pSrc += srcStride;
    pTmp += tmpStride;
    tmpWriter(*pTmp, top + mid + mid + mid);
  }

  // Horizontal pass: mid = right - left
  for (int y = 0; y < height; ++y) {
    T left(tmpReader(*tmp));

    if (width == 1) {
      dstWriter(*dst, left - left);
    } else {
      T mid(tmpReader(tmp[1]));
      dstWriter(dst[0], mid - left);

      int x = 1;
      for (; x < width - 1; ++x) {
        const T right(tmpReader(tmp[x + 1]));
        dstWriter(dst[x], right - left);
        left = mid;
        mid = right;
      }

      dstWriter(dst[x], mid - left);
    }

    tmp += tmpStride;
    dst += dstStride;
  }
}  // horizontalSobel

template <typename T,
          typename SrcIt,
          typename TmpIt,
          typename DstIt,
          typename SrcReader,
          typename TmpWriter,
          typename TmpReader,
          typename DstWriter>
void verticalSobel(const int width,
                   const int height,
                   SrcIt src,
                   int srcStride,
                   SrcReader srcReader,
                   TmpIt tmp,
                   const int tmpStride,
                   TmpWriter tmpWriter,
                   TmpReader tmpReader,
                   DstIt dst,
                   const int dstStride,
                   DstWriter dstWriter) {
  if ((width <= 0) || (height <= 0)) {
    return;
  }

  const TmpIt tmpOrig(tmp);

  // Horizontal pre-accumulation pass: mid = left + mid*2 + right
  for (int y = 0; y < height; ++y) {
    T left(srcReader(*src));

    if (width == 1) {
      tmpWriter(*tmp, left + left + left + left);
    } else {
      T mid(srcReader(src[1]));
      tmpWriter(tmp[0], left + left + left + mid);

      int x = 1;
      for (; x < width - 1; ++x) {
        const T right(srcReader(src[x + 1]));
        tmpWriter(tmp[x], left + mid + mid + right);
        left = mid;
        mid = right;
      }

      tmpWriter(tmp[x], left + mid + mid + mid);
    }
    src += srcStride;
    tmp += tmpStride;
  }

  // Vertical pass: mid = bottom - top
  for (int x = 0; x < width; ++x) {
    TmpIt pTmp(tmpOrig + x);
    TmpIt pDst(dst + x);

    T top(tmpReader(*pTmp));
    if (height == 1) {
      dstWriter(*pDst, top - top);
      continue;
    }

    T mid(tmpReader(pTmp[tmpStride]));
    dstWriter(*pDst, mid - top);

    for (int y = 1; y < height - 1; ++y) {
      pTmp += tmpStride;
      pDst += dstStride;
      const T bottom(tmpReader(pTmp[tmpStride]));
      dstWriter(*pDst, bottom - top);
      top = mid;
      mid = bottom;
    }

    pTmp += tmpStride;
    pDst += dstStride;
    dstWriter(*pDst, mid - top);
  }
}  // verticalSobel
}  // namespace imageproc
#endif  // ifndef SCANTAILOR_IMAGEPROC_SOBEL_H_
