/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C)  Joseph Artsimovich <joseph.artsimovich@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#ifndef OUTPUT_DESPECKLE_STATE_H_
#define OUTPUT_DESPECKLE_STATE_H_

#include <QImage>
#include "DespeckleLevel.h"
#include "Dpi.h"
#include "imageproc/BinaryImage.h"

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
}  // namespace output
#endif  // ifndef OUTPUT_DESPECKLE_STATE_H_
