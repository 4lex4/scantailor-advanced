// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_IMAGEPROC_FINDPEAKSGENERIC_H_
#define SCANTAILOR_IMAGEPROC_FINDPEAKSGENERIC_H_

#include <QDebug>
#include <QPoint>
#include <QRect>
#include <QSize>
#include <algorithm>
#include <cstdint>
#include <limits>
#include <vector>

#include "BinaryImage.h"
#include "Connectivity.h"
#include "LocalMinMaxGeneric.h"
#include "SeedFillGeneric.h"

namespace imageproc {
namespace detail {
namespace find_peaks {
template <typename T,
          typename MostSignificantSelector,
          typename LeastSignificantSelector,
          typename IncreaseSignificance>
void raiseAllButPeaks(MostSignificantSelector mostSignificant,
                      LeastSignificantSelector leastSignificant,
                      IncreaseSignificance increaseSignificance,
                      QSize peakNeighborhood,
                      T outsideValues,
                      const T* input,
                      int inputStride,
                      QSize size,
                      T* toBeRaised,
                      int toBeRaisedStride) {
  if (peakNeighborhood.isEmpty()) {
    peakNeighborhood.setWidth(1);
    peakNeighborhood.setHeight(1);
  }

  // Dilate the peaks and write results to seed.
  QRect neighborhood(QPoint(0, 0), peakNeighborhood);
  neighborhood.moveCenter(QPoint(0, 0));
  localMinMaxGeneric(mostSignificant, neighborhood, outsideValues, input, inputStride, size, toBeRaised,
                     toBeRaisedStride);

  std::vector<T> mask(toBeRaised, toBeRaised + toBeRaisedStride * size.height());
  const int maskStride = toBeRaisedStride;

  // Slightly raise the mask relative to toBeRaised.
  std::transform(mask.begin(), mask.end(), mask.begin(), increaseSignificance);

  seedFillGenericInPlace(mostSignificant, leastSignificant, CONN8, &toBeRaised[0], toBeRaisedStride, size, &mask[0],
                         maskStride);
}
}  // namespace find_peaks
}  // namespace detail

/**
 * \brief Finds positive or negative peaks on a 2D grid.
 *
 * A peak is defined as a cell or an 8-connected group of cells that is more
 * significant than any of its neighbor cells.  More significant means either
 * greater or less, depending on the kind of peaks we want to locate.
 * In addition, we provide functionality to suppress peaks that are
 * in a specified neighborhood of a more significant peak.
 *
 * \param mostSignificant A functor or a pointer to a free function that
 *        can be called with two arguments of type T and return the bigger
 *        or the smaller of the two.
 * \param leastSignificant Same as mostSignificant, but the oposite operation.
 * \param increaseSignificance A functor or a pointer to a free function that
 *        takes one argument and returns the next most significant value next
 *        to it.  Hint: for floating point data, use the std::nextafter() family of
 *        functions.  Their generic versions are available in Boost.
 * \param peakMutator A functor or a pointer to a free function that will
 *        transform a peak value.  Two typical cases would be returning
 *        the value as is and returning a fixed value.
 * \param nonPeakMutator Same as peakMutator, but for non-peak values.
 * \param neighborhood The area around a peak (centered at width/2, height/2)
 *        in which less significant peaks will be suppressed.  Passing an empty
 *        neighborhood is equivalent of passing a 1x1 neighborhood.
 * \param outsideValues Values that are assumed to be outside of the grid bounds.
 *        This will affect peak detection at the edges of grid.
 * \param[in,out] data Pointer to the data buffer.
 * \param stride The size of a row in the data buffer, in terms of the number of T objects.
 * \param size Grid dimensions.
 */
template <typename T,
          typename MostSignificantSelector,
          typename LeastSignificantSelector,
          typename IncreaseSignificance,
          typename PeakMutator,
          typename NonPeakMutator>
void findPeaksInPlaceGeneric(MostSignificantSelector mostSignificant,
                             LeastSignificantSelector leastSignificant,
                             IncreaseSignificance increaseSignificance,
                             PeakMutator peakMutator,
                             NonPeakMutator nonPeakMutator,
                             QSize peakNeighborhood,
                             T outsideValues,
                             T* data,
                             int stride,
                             QSize size) {
  if (size.isEmpty()) {
    return;
  }

  std::vector<T> raised(size.width() * size.height());
  const int raisedStride = size.width();

  detail::find_peaks::raiseAllButPeaks(mostSignificant, leastSignificant, increaseSignificance, peakNeighborhood,
                                       outsideValues, data, stride, size, &raised[0], raisedStride);

  T* dataLine = data;
  T* raisedLine = &raised[0];
  const int w = size.width();
  const int h = size.height();

  for (int y = 0; y < h; ++y) {
    for (int x = 0; x < w; ++x) {
      if (dataLine[x] == raisedLine[x]) {
        dataLine[x] = peakMutator(dataLine[x]);
      } else {
        dataLine[x] = nonPeakMutator(dataLine[x]);
      }
    }
    raisedLine += raisedStride;
    dataLine += stride;
  }
}  // findPeaksInPlaceGeneric

/**
 * \brief Same as findPeaksInPlaceGeneric(), but returning a binary image
 *        rather than mutating the input data.
 */
template <typename T,
          typename MostSignificantSelector,
          typename LeastSignificantSelector,
          typename IncreaseSignificance>
BinaryImage findPeaksGeneric(MostSignificantSelector mostSignificant,
                             LeastSignificantSelector leastSignificant,
                             IncreaseSignificance increaseSignificance,
                             QSize peakNeighborhood,
                             T outsideValues,
                             const T* data,
                             int stride,
                             QSize size) {
  if (size.isEmpty()) {
    return BinaryImage();
  }

  std::vector<T> raised(size.width() * size.height());
  const int raisedStride = size.width();

  detail::find_peaks::raiseAllButPeaks(mostSignificant, leastSignificant, increaseSignificance, peakNeighborhood,
                                       outsideValues, data, stride, size, &raised[0], raisedStride);

  BinaryImage peaks(size, WHITE);
  uint32_t* peaksLine = peaks.data();
  const int peaksStride = peaks.wordsPerLine();
  const T* dataLine = data;
  const T* raisedLine = &raised[0];
  const int w = size.width();
  const int h = size.height();
  const uint32_t msb = uint32_t(1) << 31;

  for (int y = 0; y < h; ++y) {
    for (int x = 0; x < w; ++x) {
      if (dataLine[x] == raisedLine[x]) {
        peaksLine[x >> 5] |= msb >> (x & 31);
      }
    }
    peaksLine += peaksStride;
    raisedLine += raisedStride;
    dataLine += stride;
  }
  return peaks;
}  // findPeaksGeneric
}  // namespace imageproc
#endif  // ifndef SCANTAILOR_IMAGEPROC_FINDPEAKSGENERIC_H_
