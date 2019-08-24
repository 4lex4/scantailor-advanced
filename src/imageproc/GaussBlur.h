// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Copyright (C) 1995 Spencer Kimball and Peter Mattis
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_IMAGEPROC_GAUSSBLUR_H_
#define SCANTAILOR_IMAGEPROC_GAUSSBLUR_H_

#include <QSize>
#include <boost/scoped_array.hpp>
#include <cstring>
#include <iterator>
#include "ValueConv.h"

namespace imageproc {
class GrayImage;

/**
 * \brief Applies gaussian blur on a GrayImage.
 *
 * \param src The image to apply gaussian blur to.
 * \param hSigma The standard deviation in horizontal direction.
 * \param vSigma The standard deviation in vertical direction.
 * \return The blurred image.
 */
GrayImage gaussBlur(const GrayImage& src, float hSigma, float vSigma);

/**
 * \brief Applies a 2D gaussian filter on an arbitrary data grid.
 *
 * \param size Data grid dimensions.
 * \param hSigma The standard deviation in horizontal direction.
 * \param vSigma The standard deviation in vertical direction.
 * \param input A random access iterator (usually a pointer)
 *        to the beginning of input data.
 * \param inputStride The distance (in terms of iterator difference)
 *        from an input grid cell to the one directly below it.
 * \param floatReader A functor to convert whatever value corresponds to *input
 *        into a float.  Consider using one of the functors from ValueConv.h
 *        The functor will be called like this:
 * \code
 * const FloatReader reader = ...;
 * const float val = reader(input[x]);
 * \endcode
 * \param output A random access iterator (usually a pointer)
 *        to the beginning of output data.  Output may point to the same
 *        memory as input.
 * \param outputStride The distance (in terms of iterator difference)
 *        from an output grid cell to the one directly below it.
 * \param floatWriter A functor that takes a float value, optionally
 *        converts it into another type and updates an output item.
 *        The functor will be called like this:
 * \code
 * const FloatWriter writer = ...;
 * const float val = ...;
 * writer(output[x], val);
 * \endcode
 * Consider using boost::lambda, possible in conjunction with one of the functors
 * from ValueConv.h:
 * \code
 * using namespace boost::lambda;
 *
 * // Just copying.
 * gaussBlurGeneric(..., _1 = _2);
 *
 * // Convert to uint8_t, with rounding and clipping.
 * gaussBlurGeneric(..., _1 = bind<uint8_t>(RoundAndClipValueConv<uint8_t>(), _2);
 * \endcode
 */
template <typename SrcIt, typename DstIt, typename FloatReader, typename FloatWriter>
void gaussBlurGeneric(QSize size,
                      float hSigma,
                      float vSigma,
                      SrcIt input,
                      int inputStride,
                      FloatReader floatReader,
                      DstIt output,
                      int outputStride,
                      FloatWriter floatWriter);

namespace gauss_blur_impl {
void findIirConstants(float* nP, float* nM, float* dP, float* dM, float* bdP, float* bdM, float stdDev);

template <typename Src1It, typename Src2It, typename DstIt, typename FloatWriter>
void save(int numItems, Src1It src1, Src2It src2, DstIt dst, int dstStride, FloatWriter writer) {
  while (numItems-- != 0) {
    writer(*dst, *src1 + *src2);
    ++src1;
    ++src2;
    dst += dstStride;
  }
}

class FloatToFloatWriter {
 public:
  void operator()(float& dst, float src) const { dst = src; }
};
}  // namespace gauss_blur_impl

template <typename SrcIt, typename DstIt, typename FloatReader, typename FloatWriter>
void gaussBlurGeneric(const QSize size,
                      const float hSigma,
                      const float vSigma,
                      const SrcIt input,
                      const int inputStride,
                      const FloatReader floatReader,
                      const DstIt output,
                      const int outputStride,
                      const FloatWriter floatWriter) {
  if (size.isEmpty()) {
    return;
  }

  const int width = size.width();
  const int height = size.height();
  const int widthHeightMax = width > height ? width : height;

  boost::scoped_array<float> valP(new float[widthHeightMax]);
  boost::scoped_array<float> valM(new float[widthHeightMax]);
  boost::scoped_array<float> intermediateImage(new float[width * height]);
  const int intermediateStride = width;

  // IIR parameters.
  float nP[5], nM[5], dP[5], dM[5], bdP[5], bdM[5];
  // Vertical pass.
  gauss_blur_impl::findIirConstants(nP, nM, dP, dM, bdP, bdM, vSigma);
  for (int x = 0; x < width; ++x) {
    memset(&valP[0], 0, height * sizeof(valP[0]));
    memset(&valM[0], 0, height * sizeof(valM[0]));

    SrcIt spP(input + x);
    SrcIt spM(spP + (height - 1) * inputStride);
    float* vp = &valP[0];
    float* vm = &valM[0] + height - 1;
    const float initialP = floatReader(spP[0]);
    const float initialM = floatReader(spM[0]);

    for (int y = 0; y < height; ++y) {
      const int terms = y < 4 ? y : 4;
      int i = 0;
      int spOff = 0;
      for (; i <= terms; ++i, spOff += inputStride) {
        *vp += nP[i] * floatReader(spP[-spOff]) - dP[i] * vp[-i];
        *vm += nM[i] * floatReader(spM[spOff]) - dM[i] * vm[i];
      }
      for (; i <= 4; ++i) {
        *vp += (nP[i] - bdP[i]) * initialP;
        *vm += (nM[i] - bdM[i]) * initialM;
      }
      spP += inputStride;
      spM -= inputStride;
      ++vp;
      --vm;
    }

    gauss_blur_impl::save(height, &valP[0], &valM[0], &intermediateImage[0] + x, intermediateStride,
                          gauss_blur_impl::FloatToFloatWriter());
  }
  // Horizontal pass.
  gauss_blur_impl::findIirConstants(nP, nM, dP, dM, bdP, bdM, hSigma);
  const float* intermediateLine = &intermediateImage[0];
  DstIt outputLine(output);
  for (int y = 0; y < height; ++y) {
    memset(&valP[0], 0, width * sizeof(valP[0]));
    memset(&valM[0], 0, width * sizeof(valM[0]));

    const float* spP = intermediateLine;
    const float* spM = intermediateLine + width - 1;
    float* vp = &valP[0];
    float* vm = &valM[0] + width - 1;
    const float initialP = spP[0];
    const float initialM = spM[0];

    for (int x = 0; x < width; ++x) {
      const int terms = x < 4 ? x : 4;
      int i = 0;
      for (; i <= terms; ++i) {
        *vp += nP[i] * spP[-i] - dP[i] * vp[-i];
        *vm += nM[i] * spM[i] - dM[i] * vm[i];
      }
      for (; i <= 4; ++i) {
        *vp += (nP[i] - bdP[i]) * initialP;
        *vm += (nM[i] - bdM[i]) * initialM;
      }
      ++spP;
      --spM;
      ++vp;
      --vm;
    }

    gauss_blur_impl::save(width, &valP[0], &valM[0], outputLine, 1, floatWriter);

    intermediateLine += intermediateStride;
    outputLine += outputStride;
  }
}  // gaussBlurGeneric
}  // namespace imageproc
#endif  // ifndef SCANTAILOR_IMAGEPROC_GAUSSBLUR_H_
