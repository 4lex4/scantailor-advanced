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

#ifndef DEWARPING_TOWARDS_LINE_TRACER_H_
#define DEWARPING_TOWARDS_LINE_TRACER_H_

#include <QLineF>
#include <QPoint>
#include <QRect>
#include <cstdint>
#include "Grid.h"
#include "VecNT.h"

namespace imageproc {
class SEDM;
}

namespace dewarping {
/**
 * This class is used for tracing a path towards intersection with a given line.
 */
class TowardsLineTracer {
 public:
  TowardsLineTracer(const imageproc::SEDM* dm, const Grid<float>* pm, const QLineF& line, const QPoint& initial_pos);

  const QPoint* trace(float max_dist);

 private:
  struct Step {
    Vec2d unitVec;
    QPoint vec;
    int dmOffset{};
    int pmOffset{};
  };

  void setupSteps();

  const uint32_t* m_pDmData;
  int m_dmStride;
  const float* m_pPmData;
  int m_pmStride;
  QRect m_rect;
  QLineF m_line;
  Vec2d m_normalTowardsLine;
  QPoint m_lastOutputPos;
  Step m_steps[5];
  int m_numSteps;
  bool m_finished;
};
}  // namespace dewarping
#endif  // ifndef DEWARPING_TOWARDS_LINE_TRACER_H_
