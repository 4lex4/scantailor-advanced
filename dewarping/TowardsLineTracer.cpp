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

#include "TowardsLineTracer.h"
#include <boost/foreach.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/lambda/lambda.hpp>
#include <cassert>
#include "NumericTraits.h"
#include "SidesOfLine.h"
#include "imageproc/SEDM.h"

using namespace imageproc;

namespace dewarping {
TowardsLineTracer::TowardsLineTracer(const imageproc::SEDM* dm,
                                     const Grid<float>* pm,
                                     const QLineF& line,
                                     const QPoint& initial_pos)
    : m_pDmData(dm->data()),
      m_dmStride(dm->stride()),
      m_pPmData(pm->data()),
      m_pmStride(pm->stride()),
      m_rect(QPoint(0, 0), dm->size()),
      m_line(line),
      m_normalTowardsLine(m_line.normalVector().p2() - m_line.p1()),
      m_lastOutputPos(initial_pos),
      m_numSteps(0),
      m_finished(false) {
  if (sidesOfLine(m_line, initial_pos, line.p1() + m_normalTowardsLine) > 0) {
    // It points the wrong way -> fix that.
    m_normalTowardsLine = -m_normalTowardsLine;
  }

  setupSteps();
}

const QPoint* TowardsLineTracer::trace(const float max_dist) {
  if (m_finished) {
    return nullptr;
  }

  const int max_sqdist = qRound(max_dist * max_dist);

  QPoint cur_pos(m_lastOutputPos);
  QPoint last_content_pos(-1, -1);

  const uint32_t* p_dm = m_pDmData + cur_pos.y() * m_dmStride + cur_pos.x();
  const float* p_pm = m_pPmData + cur_pos.y() * m_pmStride + cur_pos.x();

  for (;;) {
    int best_dm_idx = -1;
    int best_pm_idx = -1;
    uint32_t best_squared_dist = 0;
    float best_probability = NumericTraits<float>::min();

    for (int i = 0; i < m_numSteps; ++i) {
      const Step& step = m_steps[i];
      const QPoint new_pos(cur_pos + step.vec);
      if (!m_rect.contains(new_pos)) {
        continue;
      }

      const uint32_t sqd = p_dm[step.dmOffset];
      if (sqd > best_squared_dist) {
        best_squared_dist = sqd;
        best_dm_idx = i;
      }
      const float probability = p_pm[step.pmOffset];
      if (probability > best_probability) {
        best_probability = probability;
        best_pm_idx = i;
      }
    }

    if (best_dm_idx == -1) {
      m_finished = true;
      break;
    }
    assert(best_pm_idx != -1);

    int best_idx = best_pm_idx;
    if (p_dm[m_steps[best_dm_idx].dmOffset] > *p_dm) {
      best_idx = best_dm_idx;
    }

    Step& step = m_steps[best_idx];

    if (sidesOfLine(m_line, cur_pos + step.vec, m_lastOutputPos) < 0) {
      // Note that this has to be done before we update cur_pos,
      // as it will be used after breaking from this loop.
      m_finished = true;
      break;
    }

    cur_pos += step.vec;
    p_dm += step.dmOffset;
    p_pm += step.pmOffset;

    const QPoint vec(cur_pos - m_lastOutputPos);
    if (vec.x() * vec.x() + vec.y() * vec.y() > max_sqdist) {
      m_lastOutputPos = cur_pos;

      return &m_lastOutputPos;
    }
  }

  if (m_lastOutputPos != cur_pos) {
    m_lastOutputPos = cur_pos;

    return &m_lastOutputPos;
  } else {
    return nullptr;
  }
}  // TowardsLineTracer::trace

void TowardsLineTracer::setupSteps() {
  QPoint all_directions[8];
  // all_directions[0] is north-west, and then clockwise from there.
  static const int m0p[] = {-1, 0, 1};
  static const int p0m[] = {1, 0, -1};

  for (int i = 0; i < 3; ++i) {
    // north
    all_directions[i].setX(m0p[i]);
    all_directions[i].setY(-1);

    // east
    all_directions[2 + i].setX(1);
    all_directions[2 + i].setY(m0p[i]);

    // south
    all_directions[4 + i].setX(p0m[i]);
    all_directions[4 + i].setY(1);

    // west
    all_directions[(6 + i) & 7].setX(-1);
    all_directions[(6 + i) & 7].setY(p0m[i]);
  }

  m_numSteps = 0;
  for (const QPoint dir : all_directions) {
    if (m_normalTowardsLine.dot(QPointF(dir)) > 0.0) {
      Step& step = m_steps[m_numSteps];
      step.vec = dir;
      step.unitVec = Vec2d(step.vec.x(), step.vec.y());
      step.unitVec /= std::sqrt(step.unitVec.squaredNorm());
      step.dmOffset = step.vec.y() * m_dmStride + step.vec.x();
      step.pmOffset = step.vec.y() * m_pmStride + step.vec.x();
      ++m_numSteps;
      assert(m_numSteps <= int(sizeof(m_steps) / sizeof(m_steps[0])));
    }
  }

  // Sort by decreasing alignment with m_normalTowardsLine.
  using namespace boost::lambda;
  std::sort(m_steps, m_steps + m_numSteps,
            bind(&Vec2d::dot, m_normalTowardsLine, bind<const Vec2d&>(&Step::unitVec, _1))
                > bind(&Vec2d::dot, m_normalTowardsLine, bind<const Vec2d&>(&Step::unitVec, _2)));
}  // TowardsLineTracer::setupSteps
}  // namespace dewarping