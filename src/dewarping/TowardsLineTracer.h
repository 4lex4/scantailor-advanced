// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_DEWARPING_TOWARDSLINETRACER_H_
#define SCANTAILOR_DEWARPING_TOWARDSLINETRACER_H_

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
  TowardsLineTracer(const imageproc::SEDM* dm, const Grid<float>* pm, const QLineF& line, const QPoint& initialPos);

  const QPoint* trace(float maxDist);

 private:
  struct Step {
    Vec2d unitVec;
    QPoint vec;
    int dmOffset{};
    int pmOffset{};
  };

  void setupSteps();

  const uint32_t* m_dmData;
  int m_dmStride;
  const float* m_pmData;
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
#endif  // ifndef SCANTAILOR_DEWARPING_TOWARDSLINETRACER_H_
