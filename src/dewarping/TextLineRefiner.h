// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_DEWARPING_TEXTLINEREFINER_H_
#define SCANTAILOR_DEWARPING_TEXTLINEREFINER_H_

#include <GrayImage.h>

#include <QLineF>
#include <QPointF>
#include <cstdint>
#include <list>
#include <vector>

#include "Dpi.h"
#include "Grid.h"
#include "VecNT.h"

class Dpi;
class DebugImages;
class QImage;

namespace dewarping {
class TextLineRefiner {
 public:
  TextLineRefiner(const imageproc::GrayImage& image, const Dpi& dpi, const Vec2f& unitDownVector);

  void refine(std::list<std::vector<QPointF>>& polylines, int iterations, DebugImages* dbg) const;

 private:
  enum OnConvergence { ON_CONVERGENCE_STOP, ON_CONVERGENCE_GO_FINER };

  class SnakeLength;

  struct FrenetFrame;

  class Optimizer;

  struct SnakeNode {
    Vec2f center;
    float ribHalfLength{};
  };

  struct Snake {
    std::vector<SnakeNode> nodes;
    int iterationsRemaining;

    Snake() : iterationsRemaining(0) {}
  };

  struct Step {
    SnakeNode node;
    uint32_t prevStepIdx{0};
    float pathCost{0};
  };

  void calcBlurredGradient(Grid<float>& gradient, float hSigma, float vSigma) const;

  static float externalEnergyAt(const Grid<float>& gradient, const Vec2f& pos, float penaltyIfOutside);

  static Snake makeSnake(const std::vector<QPointF>& polyline, int iterations);

  static void calcFrenetFrames(std::vector<FrenetFrame>& frenetFrames,
                               const Snake& snake,
                               const SnakeLength& snakeLength,
                               const Vec2f& unitDownVec);

  void evolveSnake(Snake& snake, const Grid<float>& gradient, OnConvergence onConvergence) const;

  QImage visualizeGradient(const Grid<float>& gradient) const;

  QImage visualizeSnakes(const std::vector<Snake>& snakes, const Grid<float>* gradient = nullptr) const;

  imageproc::GrayImage m_image;
  Dpi m_dpi;
  Vec2f m_unitDownVec;
};
}  // namespace dewarping
#endif  // ifndef SCANTAILOR_DEWARPING_TEXTLINEREFINER_H_
