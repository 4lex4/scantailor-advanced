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

#ifndef DEWARPING_TOP_BOTTOM_EDGE_TRACER_H_
#define DEWARPING_TOP_BOTTOM_EDGE_TRACER_H_

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

  static int initNeighbours(int* next_nbh_offsets, int* prev_nbh_indexes, int stride, const Vec2f& direction);

  static std::vector<QPoint> locateBestPathEndpoints(const Grid<GridNode>& grid, const QLineF& line);

  static std::vector<QPoint> tracePathFromEndpoint(const Grid<GridNode>& grid, const QPoint& endpoint);

  static std::vector<QPointF> pathToSnake(const Grid<GridNode>& grid, const QPoint& endpoint);

  static void gaussBlurGradient(Grid<GridNode>& grid);

  static Vec2f downTheHillDirection(const QRectF& page_rect,
                                    const std::vector<QPointF>& snake,
                                    const Vec2f& bounds_dir);

  static void downTheHillSnake(std::vector<QPointF>& snake, const Grid<GridNode>& grid, Vec2f dir);

  static void upTheHillSnake(std::vector<QPointF>& snake, const Grid<GridNode>& grid, Vec2f dir);

  static int initDisplacementVectors(Vec2f vectors[], Vec2f valid_direction);

  template <typename Extractor>
  static float interpolatedGridValue(const Grid<GridNode>& grid, Extractor extractor, Vec2f pos, float default_value);

  static QImage visualizeGradient(const Grid<GridNode>& grid, const QImage* background = nullptr);

  static QImage visualizeBlurredGradient(const Grid<GridNode>& grid);

  static QImage visualizePaths(const QImage& background,
                               const Grid<GridNode>& grid,
                               const std::pair<QLineF, QLineF>& bounds,
                               const std::vector<QPoint>& path_endpoints);

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
#endif  // ifndef DEWARPING_TOP_BOTTOM_EDGE_TRACER_H_
