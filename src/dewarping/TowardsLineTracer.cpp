// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "TowardsLineTracer.h"
#include <SEDM.h>
#include <boost/foreach.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/lambda/lambda.hpp>
#include <cassert>
#include "NumericTraits.h"
#include "SidesOfLine.h"

using namespace imageproc;

namespace dewarping {
TowardsLineTracer::TowardsLineTracer(const imageproc::SEDM* dm,
                                     const Grid<float>* pm,
                                     const QLineF& line,
                                     const QPoint& initialPos)
    : m_dmData(dm->data()),
      m_dmStride(dm->stride()),
      m_pmData(pm->data()),
      m_pmStride(pm->stride()),
      m_rect(QPoint(0, 0), dm->size()),
      m_line(line),
      m_normalTowardsLine(m_line.normalVector().p2() - m_line.p1()),
      m_lastOutputPos(initialPos),
      m_numSteps(0),
      m_finished(false) {
  if (sidesOfLine(m_line, initialPos, line.p1() + m_normalTowardsLine) > 0) {
    // It points the wrong way -> fix that.
    m_normalTowardsLine = -m_normalTowardsLine;
  }

  setupSteps();
}

const QPoint* TowardsLineTracer::trace(const float maxDist) {
  if (m_finished) {
    return nullptr;
  }

  const int maxSqdist = qRound(maxDist * maxDist);

  QPoint curPos(m_lastOutputPos);
  QPoint lastContentPos(-1, -1);

  const uint32_t* pDm = m_dmData + curPos.y() * m_dmStride + curPos.x();
  const float* pPm = m_pmData + curPos.y() * m_pmStride + curPos.x();

  while (true) {
    int bestDmIdx = -1;
    int bestPmIdx = -1;
    uint32_t bestSquaredDist = 0;
    float bestProbability = NumericTraits<float>::min();

    for (int i = 0; i < m_numSteps; ++i) {
      const Step& step = m_steps[i];
      const QPoint newPos(curPos + step.vec);
      if (!m_rect.contains(newPos)) {
        continue;
      }

      const uint32_t sqd = pDm[step.dmOffset];
      if (sqd > bestSquaredDist) {
        bestSquaredDist = sqd;
        bestDmIdx = i;
      }
      const float probability = pPm[step.pmOffset];
      if (probability > bestProbability) {
        bestProbability = probability;
        bestPmIdx = i;
      }
    }

    if (bestDmIdx == -1) {
      m_finished = true;
      break;
    }
    assert(bestPmIdx != -1);

    int bestIdx = bestPmIdx;
    if (pDm[m_steps[bestDmIdx].dmOffset] > *pDm) {
      bestIdx = bestDmIdx;
    }

    Step& step = m_steps[bestIdx];

    if (sidesOfLine(m_line, curPos + step.vec, m_lastOutputPos) < 0) {
      // Note that this has to be done before we update curPos,
      // as it will be used after breaking from this loop.
      m_finished = true;
      break;
    }

    curPos += step.vec;
    pDm += step.dmOffset;
    pPm += step.pmOffset;

    const QPoint vec(curPos - m_lastOutputPos);
    if (vec.x() * vec.x() + vec.y() * vec.y() > maxSqdist) {
      m_lastOutputPos = curPos;

      return &m_lastOutputPos;
    }
  }

  if (m_lastOutputPos != curPos) {
    m_lastOutputPos = curPos;

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