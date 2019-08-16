// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "DespeckleState.h"
#include "DebugImages.h"
#include "Despeckle.h"
#include "DespeckleVisualization.h"
#include "TaskStatus.h"
#include <RasterOp.h>

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
  DespeckleState new_state(*this);

  if (level == m_despeckleLevel) {
    return new_state;
  }

  new_state.m_despeckleLevel = level;

  if (level == 0) {
    // Null speckles image is equivalent to a white one.
    new_state.m_speckles.release();

    return new_state;
  }

  new_state.m_speckles = Despeckle::despeckle(m_everythingBW, m_dpi, level, status, dbg);

  status.throwIfCancelled();

  rasterOp<RopSubtract<RopSrc, RopDst>>(new_state.m_speckles, m_everythingBW);

  return new_state;
}  // DespeckleState::redespeckle

QImage DespeckleState::overlaySpeckles(const QImage& mixed, const imageproc::BinaryImage& speckles) {
  QImage result(mixed.convertToFormat(QImage::Format_RGB32));
  if (result.isNull() && !mixed.isNull()) {
    throw std::bad_alloc();
  }

  if (speckles.isNull()) {
    return result;
  }

  auto* result_line = (uint32_t*) result.bits();
  const int result_stride = result.bytesPerLine() / 4;

  const uint32_t* speckles_line = speckles.data();
  const int speckles_stride = speckles.wordsPerLine();
  const uint32_t msb = uint32_t(1) << 31;

  const int width = result.width();
  const int height = result.height();

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      if (speckles_line[x >> 5] & (msb >> (x & 31))) {
        result_line[x] = 0xff000000;  // opaque black
      }
    }
    result_line += result_stride;
    speckles_line += speckles_stride;
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

  const auto* mixed_line = (const uint32_t*) mixed.bits();
  const int mixed_stride = mixed.bytesPerLine() / 4;

  uint32_t* result_line = result.data();
  const int result_stride = result.wordsPerLine();
  const uint32_t msb = uint32_t(1) << 31;

  const int width = result.width();
  const int height = result.height();

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      if (mixed_line[x] == 0xff000000) {
        result_line[x >> 5] |= msb >> (x & 31);
      }
    }
    mixed_line += mixed_stride;
    result_line += result_stride;
  }

  return result;
}

double DespeckleState::level() const {
  return m_despeckleLevel;
}
}  // namespace output