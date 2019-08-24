// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_IMAGEPROC_LOCALMINMAXGENERIC_H_
#define SCANTAILOR_IMAGEPROC_LOCALMINMAXGENERIC_H_

#include <QRect>
#include <QSize>
#include <algorithm>
#include <cassert>
#include <vector>

namespace imageproc {
namespace detail {
namespace local_min_max {
template <typename T, typename MinMaxSelector>
void fillAccumulator(MinMaxSelector selector,
                     int todoBefore,
                     int todoWithin,
                     int todoAfter,
                     T outsideValues,
                     const T* src,
                     const int srcDelta,
                     T* dst,
                     const int dstDelta) {
  T extremum(outsideValues);

  if ((todoBefore <= 0) && (todoWithin > 0)) {
    extremum = *src;
  }

  while (todoBefore-- > 0) {
    *dst = extremum;
    dst += dstDelta;
    src += srcDelta;
  }

  while (todoWithin-- > 0) {
    extremum = selector(extremum, *src);
    *dst = extremum;
    src += srcDelta;
    dst += dstDelta;
  }

  if (todoAfter > 0) {
    extremum = selector(extremum, outsideValues);
    do {
      *dst = extremum;
      dst += dstDelta;
    } while (--todoAfter > 0);
  }
}

template <typename T>
void fillWithConstant(T* from, T* to, T constant) {
  for (++to; from != to; ++from) {
    *from = constant;
  }
}

template <typename T, typename MinMaxSelector>
void horizontalPass(MinMaxSelector selector,
                    const QRect neighborhood,
                    const T outsideValues,
                    const T* input,
                    const int inputStride,
                    const QSize inputSize,
                    T* output,
                    const int outputStride) {
  const int seLen = neighborhood.width();
  const int width = inputSize.width();
  const int widthM1 = width - 1;
  const int height = inputSize.height();
  const int dx1 = neighborhood.left();
  const int dx2 = neighborhood.right();

  std::vector<T> accum(seLen * 2 - 1);
  T* const accumMiddle = &accum[seLen - 1];

  for (int y = 0; y < height; ++y) {
    for (int dstSegmentFirst = 0; dstSegmentFirst < width; dstSegmentFirst += seLen) {
      const int dstSegmentLast = std::min(dstSegmentFirst + seLen, width) - 1;  // inclusive
      const int srcSegmentFirst = dstSegmentFirst + dx1;
      const int srcSegmentLast = dstSegmentLast + dx2;
      const int srcSegmentMiddle = (srcSegmentFirst + srcSegmentLast) >> 1;
      // Fill the first half of the accumulator buffer.
      if ((srcSegmentFirst > widthM1) || (srcSegmentMiddle < 0)) {
        // This half is completely outside the image.
        // Note that the branch below can't deal with such a case.
        fillWithConstant(&accum.front(), accumMiddle, outsideValues);
      } else {
        // after <- [to <- within <- from] <- before
        const int from = std::min(widthM1, srcSegmentMiddle);
        const int to = std::max(0, srcSegmentFirst);

        const int todoBefore = srcSegmentMiddle - from;
        const int todoWithin = from - to + 1;
        const int todoAfter = to - srcSegmentFirst;
        const int srcDelta = -1;
        const int dstDelta = -1;

        fillAccumulator(selector, todoBefore, todoWithin, todoAfter, outsideValues, input + srcSegmentMiddle, srcDelta,
                        accumMiddle, dstDelta);
      }
      // Fill the second half of the accumulator buffer.
      if ((srcSegmentLast < 0) || (srcSegmentMiddle > widthM1)) {
        // This half is completely outside the image.
        // Note that the branch below can't deal with such a case.
        fillWithConstant(accumMiddle, &accum.back(), outsideValues);
      } else {
        // before -> [from -> within -> to] -> after
        const int from = std::max(0, srcSegmentMiddle);
        const int to = std::min(widthM1, srcSegmentLast);

        const int todoBefore = from - srcSegmentMiddle;
        const int todoWithin = to - from + 1;
        const int todoAfter = srcSegmentLast - to;
        const int srcDelta = 1;
        const int dstDelta = 1;

        fillAccumulator(selector, todoBefore, todoWithin, todoAfter, outsideValues, input + srcSegmentMiddle, srcDelta,
                        accumMiddle, dstDelta);
      }

      const int offset1 = dx1 - srcSegmentMiddle;
      const int offset2 = dx2 - srcSegmentMiddle;
      for (int x = dstSegmentFirst; x <= dstSegmentLast; ++x) {
        output[x] = selector(accumMiddle[x + offset1], accumMiddle[x + offset2]);
      }
    }

    input += inputStride;
    output += outputStride;
  }
}  // horizontalPass

template <typename T, typename MinMaxSelector>
void verticalPass(MinMaxSelector selector,
                  const QRect neighborhood,
                  const T outsideValues,
                  const T* input,
                  const int inputStride,
                  const QSize inputSize,
                  T* output,
                  const int outputStride) {
  const int seLen = neighborhood.height();
  const int width = inputSize.width();
  const int height = inputSize.height();
  const int heightM1 = height - 1;
  const int dy1 = neighborhood.top();
  const int dy2 = neighborhood.bottom();

  std::vector<T> accum(seLen * 2 - 1);
  T* const accumMiddle = &accum[seLen - 1];

  for (int x = 0; x < width; ++x) {
    for (int dstSegmentFirst = 0; dstSegmentFirst < height; dstSegmentFirst += seLen) {
      const int dstSegmentLast = std::min(dstSegmentFirst + seLen, height) - 1;  // inclusive
      const int srcSegmentFirst = dstSegmentFirst + dy1;
      const int srcSegmentLast = dstSegmentLast + dy2;
      const int srcSegmentMiddle = (srcSegmentFirst + srcSegmentLast) >> 1;
      // Fill the first half of accumulator buffer.
      if ((srcSegmentFirst > heightM1) || (srcSegmentMiddle < 0)) {
        // This half is completely outside the image.
        // Note that the branch below can't deal with such a case.
        fillWithConstant(&accum.front(), accumMiddle, outsideValues);
      } else {
        // after <- [to <- within <- from] <- before
        const int from = std::min(heightM1, srcSegmentMiddle);
        const int to = std::max(0, srcSegmentFirst);

        const int todoBefore = srcSegmentMiddle - from;
        const int todoWithin = from - to + 1;
        const int todoAfter = to - srcSegmentFirst;
        const int srcDelta = -inputStride;
        const int dstDelta = -1;

        fillAccumulator(selector, todoBefore, todoWithin, todoAfter, outsideValues,
                        input + srcSegmentMiddle * inputStride, srcDelta, accumMiddle, dstDelta);
      }
      // Fill the second half of accumulator buffer.
      if ((srcSegmentLast < 0) || (srcSegmentMiddle > heightM1)) {
        // This half is completely outside the image.
        // Note that the branch below can't deal with such a case.
        fillWithConstant(accumMiddle, &accum.back(), outsideValues);
      } else {
        // before -> [from -> within -> to] -> after
        const int from = std::max(0, srcSegmentMiddle);
        const int to = std::min(heightM1, srcSegmentLast);

        const int todoBefore = from - srcSegmentMiddle;
        const int todoWithin = to - from + 1;
        const int todoAfter = srcSegmentLast - to;
        const int srcDelta = inputStride;
        const int dstDelta = 1;

        fillAccumulator(selector, todoBefore, todoWithin, todoAfter, outsideValues,
                        input + srcSegmentMiddle * inputStride, srcDelta, accumMiddle, dstDelta);
      }

      const int offset1 = dy1 - srcSegmentMiddle;
      const int offset2 = dy2 - srcSegmentMiddle;
      T* pOut = output + dstSegmentFirst * outputStride;
      for (int y = dstSegmentFirst; y <= dstSegmentLast; ++y) {
        *pOut = selector(accumMiddle[y + offset1], accumMiddle[y + offset2]);
        pOut += outputStride;
      }
    }

    ++input;
    ++output;
  }
}  // verticalPass
}  // namespace local_min_max
}  // namespace detail

/**
 * \brief For each cell on a 2D grid, finds the minimum or the maximum value
 *        in a rectangular neighborhood.
 *
 * This can be seen as a generalized version of grayscale erode and dilate operations.
 *
 * \param selector A functor or a pointer to a free function that can be called with
 *        two arguments of type T and return the bigger or the smaller of the two.
 * \param neighborhood The rectangular neighborhood to search for maximum or minimum values.
 *        The (0, 0) point would usually be located at the center of the neighborhood
 *        rectangle, although it's not strictly required.  The neighborhood rectangle
 *        can't be empty.
 * \param outsideValues Values that are assumed to be outside of the grid bounds.
 * \param input Pointer to the input buffer.
 * \param inputStride The size of a row in input buffer, in terms of the number of T objects.
 * \param inputSize Dimensions of the input grid.
 * \param output Pointer to the output data.  Note that the input and output buffers
 *        may be the same.
 * \param outputStride The size of a row in the output buffer, in terms of the number of T objects.
 *        The output grid is presumed to have the same dimensions as the input grid.
 *
 * This code is an implementation of the following algorithm:\n
 * A fast algorithm for local minimum and maximum filters on rectangular and octagonal kernels,
 * Patt. Recog. Letters, 13, pp. 517-521, 1992
 *
 * A good description of this algorithm is available online at:
 * http://leptonica.com/grayscale-morphology.html#FAST-IMPLEMENTATION
 */
template <typename T, typename MinMaxSelector>
void localMinMaxGeneric(MinMaxSelector selector,
                        const QRect neighborhood,
                        const T outsideValues,
                        const T* input,
                        const int inputStride,
                        const QSize inputSize,
                        T* output,
                        const int outputStride) {
  assert(!neighborhood.isEmpty());

  if (inputSize.isEmpty()) {
    return;
  }

  std::vector<T> temp(inputSize.width() * inputSize.height());
  const int tempStride = inputSize.width();

  detail::local_min_max::horizontalPass(selector, neighborhood, outsideValues, input, inputStride, inputSize, &temp[0],
                                        tempStride);

  detail::local_min_max::verticalPass(selector, neighborhood, outsideValues, &temp[0], tempStride, inputSize, output,
                                      outputStride);
}
}  // namespace imageproc
#endif  // ifndef SCANTAILOR_IMAGEPROC_LOCALMINMAXGENERIC_H_
