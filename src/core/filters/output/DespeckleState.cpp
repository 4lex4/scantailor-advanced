// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "DespeckleState.h"

#include <RasterOp.h>

#include "DebugImages.h"
#include "Despeckle.h"
#include "DespeckleVisualization.h"
#include "TaskStatus.h"

using namespace imageproc;

namespace output {
DespeckleState::DespeckleState(const QImage& output,
                               const imageproc::BinaryImage& speckles,
                               const double level,
                               const Dpi& dpi)
    : m_speckles(speckles), m_dpi(dpi), m_despeckleLevel(level) {
  m_everythingMixed = overlaySpeckles(output, speckles);
  m_everythingBW = extractBW(m_everythingMixed);
}

DespeckleVisualization DespeckleState::visualize() const {
  return DespeckleVisualization(m_everythingMixed, m_speckles, m_dpi);
}

DespeckleState DespeckleState::redespeckle(const double level, const TaskStatus& status, DebugImages* dbg) const {
  DespeckleState newState(*this);

  if (level == m_despeckleLevel) {
    return newState;
  }

  newState.m_despeckleLevel = level;

  if (level == 0) {
    // Null speckles image is equivalent to a white one.
    newState.m_speckles.release();
    return newState;
  }

  newState.m_speckles = Despeckle::despeckle(m_everythingBW, m_dpi, level, status, dbg);

  status.throwIfCancelled();

  rasterOp<RopSubtract<RopSrc, RopDst>>(newState.m_speckles, m_everythingBW);
  return newState;
}  // DespeckleState::redespeckle

QImage DespeckleState::overlaySpeckles(const QImage& mixed, const imageproc::BinaryImage& speckles) {
  QImage result(mixed.convertToFormat(QImage::Format_RGB32));
  if (result.isNull() && !mixed.isNull()) {
    throw std::bad_alloc();
  }

  if (speckles.isNull()) {
    return result;
  }

  auto* resultLine = (uint32_t*) result.bits();
  const int resultStride = result.bytesPerLine() / 4;

  const uint32_t* specklesLine = speckles.data();
  const int specklesStride = speckles.wordsPerLine();
  const uint32_t msb = uint32_t(1) << 31;

  const int width = result.width();
  const int height = result.height();

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      if (specklesLine[x >> 5] & (msb >> (x & 31))) {
        resultLine[x] = 0xff000000;  // opaque black
      }
    }
    resultLine += resultStride;
    specklesLine += specklesStride;
  }
  return result;
}  // DespeckleState::overlaySpeckles

/**
 * Here we assume that B/W content have all their color components
 * set to either 0x00 or 0xff.  We enforce this convention when
 * generating output files.
 */
BinaryImage DespeckleState::extractBW(const QImage& mixed) {
  BinaryImage result(mixed.size(), WHITE);

  const auto* mixedLine = (const uint32_t*) mixed.bits();
  const int mixedStride = mixed.bytesPerLine() / 4;

  uint32_t* resultLine = result.data();
  const int resultStride = result.wordsPerLine();
  const uint32_t msb = uint32_t(1) << 31;

  const int width = result.width();
  const int height = result.height();

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      if (mixedLine[x] == 0xff000000) {
        resultLine[x >> 5] |= msb >> (x & 31);
      }
    }
    mixedLine += mixedStride;
    resultLine += resultStride;
  }
  return result;
}
}  // namespace output