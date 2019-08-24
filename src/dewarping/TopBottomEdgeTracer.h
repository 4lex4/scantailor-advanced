// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_DEWARPING_TOPBOTTOMEDGETRACER_H_
#define SCANTAILOR_DEWARPING_TOPBOTTOMEDGETRACER_H_

#include <QLineF>
#include <QPointF>
#include <QRectF>
#include <list>
#include <utility>
#include <vector>
#include "Grid.h"
#include "VecNT.h"

class TaskStatus;
class DebugImages;
class QImage;
class QPoint;

namespace imageproc {
class GrayImage;
}

namespace dewarping {
class DistortionModelBuilder;

class TopBottomEdgeTracer {
 public:
  static void trace(const imageproc::GrayImage& image,
                    std::pair<QLineF, QLineF> bounds,
                    DistortionModelBuilder& output,
                    const TaskStatus& status,
                    DebugImages* dbg = nullptr);

 private:
  struct GridNode;

  class PrioQueue;

  struct Step;

  static bool intersectWithRect(std::pair<QLineF, QLineF>& bounds, const QRectF& rect);

  static void forceSameDirection(std::pair<QLineF, QLineF>& bounds);

  static void calcDirectionalDerivative(Grid<GridNode>& gradient,
                                        const imageproc::GrayImage& image,
                                        const Vec2f& direction);

  static void horizontalSobelInPlace(Grid<GridNode>& grid);

  static void verticalSobelInPlace(Grid<GridNode>& grid);

  static Vec2f calcAvgUnitVector(const std::pair<QLineF, QLineF>& bounds);

  static Vec2f directionFromPointToLine(const QPointF& pt, const QLineF& line);

  static void prepareForShortestPathsFrom(PrioQueue& queue, Grid<GridNode>& grid, const QLineF& from);

  static void propagateShortestPaths(const Vec2f& direction, PrioQueue& queue, Grid<GridNode>& grid);

  static int initNeighbours(int* nextNbhOffsets, int* prevNbhIndexes, int stride, const Vec2f& direction);

  static std::vector<QPoint> locateBestPathEndpoints(const Grid<GridNode>& grid, const QLineF& line);

  static std::vector<QPoint> tracePathFromEndpoint(const Grid<GridNode>& grid, const QPoint& endpoint);

  static std::vector<QPointF> pathToSnake(const Grid<GridNode>& grid, const QPoint& endpoint);

  static void gaussBlurGradient(Grid<GridNode>& grid);

  static Vec2f downTheHillDirection(const QRectF& pageRect, const std::vector<QPointF>& snake, const Vec2f& boundsDir);

  static void downTheHillSnake(std::vector<QPointF>& snake, const Grid<GridNode>& grid, Vec2f dir);

  static void upTheHillSnake(std::vector<QPointF>& snake, const Grid<GridNode>& grid, Vec2f dir);

  static int initDisplacementVectors(Vec2f vectors[], Vec2f validDirection);

  template <typename Extractor>
  static float interpolatedGridValue(const Grid<GridNode>& grid, Extractor extractor, Vec2f pos, float defaultValue);

  static QImage visualizeGradient(const Grid<GridNode>& grid, const QImage* background = nullptr);

  static QImage visualizeBlurredGradient(const Grid<GridNode>& grid);

  static QImage visualizePaths(const QImage& background,
                               const Grid<GridNode>& grid,
                               const std::pair<QLineF, QLineF>& bounds,
                               const std::vector<QPoint>& pathEndpoints);

  static QImage visualizePaths(const QImage& background,
                               const std::vector<std::vector<QPoint>>& paths,
                               const std::pair<QLineF, QLineF>& bounds);

  static QImage visualizeSnakes(const QImage& background,
                                const std::vector<std::vector<QPointF>>& snakes,
                                const std::pair<QLineF, QLineF>& bounds);

  static QImage visualizePolylines(const QImage& background,
                                   const std::list<std::vector<QPointF>>& snakes,
                                   const std::pair<QLineF, QLineF>& bounds);
};
}  // namespace dewarping
#endif  // ifndef SCANTAILOR_DEWARPING_TOPBOTTOMEDGETRACER_H_
