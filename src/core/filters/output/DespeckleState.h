// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.


#ifndef SCANTAILOR_OUTPUT_DESPECKLESTATE_H_
#define SCANTAILOR_OUTPUT_DESPECKLESTATE_H_

#include <BinaryImage.h>

#include <QImage>

#include "DespeckleLevel.h"
#include "Dpi.h"

class TaskStatus;
class DebugImages;

namespace output {
class DespeckleVisualization;

/**
 * Holds enough information to build a DespeckleVisualization
 * or to re-despeckle with different DespeckleLevel.
 */
class DespeckleState {
  // Member-wise copying is OK.
 public:
  DespeckleState(const QImage& output, const imageproc::BinaryImage& speckles, double level, const Dpi& dpi);

  double level() const;

  DespeckleVisualization visualize() const;

  DespeckleState redespeckle(double level, const TaskStatus& status, DebugImages* dbg = nullptr) const;

 private:
  static QImage overlaySpeckles(const QImage& mixed, const imageproc::BinaryImage& speckles);

  static imageproc::BinaryImage extractBW(const QImage& mixed);

  /**
   * This image is the output image produced by OutputGenerator
   * with speckles added as black regions.  This image is always in RGB32,
   * because it only exists for display purposes, namely for being fed to
   * DespeckleVisualization.
   */
  QImage m_everythingMixed;

  /**
   * The B/W part of m_everythingMixed.
   */
  imageproc::BinaryImage m_everythingBW;

  /**
   * The speckles detected in m_everythingBW.
   * This image may be null, which is equivalent to having it all white.
   */
  imageproc::BinaryImage m_speckles;

  /**
   * The DPI of all 3 above images.
   */
  Dpi m_dpi;

  /**
   * Despeckling level at which m_speckles was produced from
   * m_everythingBW.
   */
  double m_despeckleLevel;
};


inline double DespeckleState::level() const {
  return m_despeckleLevel;
}
}  // namespace output
#endif  // ifndef SCANTAILOR_OUTPUT_DESPECKLESTATE_H_
