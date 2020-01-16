// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "TopBottomEdgeTracer.h"
#include <Constants.h>
#include <GaussBlur.h>
#include <GrayImage.h>
#include <Scale.h>
#include <QDebug>
#include <QPainter>
#include <boost/foreach.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/lambda/lambda.hpp>
#include <cmath>
#include "DebugImages.h"
#include "DistortionModelBuilder.h"
#include "GridLineTraverser.h"
#include "LineBoundedByRect.h"
#include "MatrixCalc.h"
#include "NumericTraits.h"
#include "PriorityQueue.h"
#include "TaskStatus.h"
#include "ToLineProjector.h"

using namespace imageproc;

namespace dewarping {
struct TopBottomEdgeTracer::GridNode {
 private:
  static const uint32_t HEAP_IDX_BITS = 28;
  static const uint32_t PREV_NEIGHBOUR_BITS = 3;
  static const uint32_t PATH_CONTINUATION_BITS = 1;

  static const uint32_t HEAP_IDX_SHIFT = 0;
  static const uint32_t PREV_NEIGHBOUR_SHIFT = HEAP_IDX_SHIFT + HEAP_IDX_BITS;
  static const uint32_t PATH_CONTINUATION_SHIFT = PREV_NEIGHBOUR_SHIFT + PREV_NEIGHBOUR_BITS;

  static const uint32_t HEAP_IDX_MASK = ((uint32_t(1) << HEAP_IDX_BITS) - uint32_t(1)) << HEAP_IDX_SHIFT;
  static const uint32_t PREV_NEIGHBOUR_MASK = ((uint32_t(1) << PREV_NEIGHBOUR_BITS) - uint32_t(1))
                                              << PREV_NEIGHBOUR_SHIFT;
  static const uint32_t PATH_CONTINUATION_MASK = ((uint32_t(1) << PATH_CONTINUATION_BITS) - uint32_t(1))
                                                 << PATH_CONTINUATION_SHIFT;

 public:
  static const uint32_t INVALID_HEAP_IDX = HEAP_IDX_MASK >> HEAP_IDX_SHIFT;

  union {
    float dirDeriv;  // Directional derivative.
    float xGrad;     // x component of the gradient.
  };

  union {
    float pathCost;
    float blurred;
    float yGrad;  // y component of the gradient.
  };

  // Note: xGrad and yGrad are used to calculate the directional
  // derivative, which then gets stored in dirDeriv.  Obviously,
  // pathCost gets overwritten, which is not a problem in our case.
  uint32_t packedData;

  float absDirDeriv() const { return std::fabs(dirDeriv); }

  void setupForPadding() {
    dirDeriv = 0;
    pathCost = -1;
    packedData = INVALID_HEAP_IDX;
  }

  /**
   * Note that is one doesn't modify dirDeriv.
   */
  void setupForInterior() {
    pathCost = NumericTraits<float>::max();
    packedData = INVALID_HEAP_IDX;
  }

  uint32_t heapIdx() const { return (packedData & HEAP_IDX_MASK) >> HEAP_IDX_SHIFT; }

  void setHeapIdx(uint32_t idx) {
    assert(!(idx & ~(HEAP_IDX_MASK >> HEAP_IDX_SHIFT)));
    packedData = idx | (packedData & ~HEAP_IDX_MASK);
  }

  bool hasPathContinuation() const { return static_cast<bool>(packedData & PATH_CONTINUATION_MASK); }

  /**
   * Neibhgours are indexed like this:
   * 0 1 2
   * 3   4
   * 5 6 7
   */
  uint32_t prevNeighbourIdx() const { return (packedData & PREV_NEIGHBOUR_MASK) >> PREV_NEIGHBOUR_SHIFT; }

  void setPrevNeighbourIdx(uint32_t idx) {
    assert(!(idx & ~(PREV_NEIGHBOUR_MASK >> PREV_NEIGHBOUR_SHIFT)));
    packedData = PATH_CONTINUATION_MASK | (idx << PREV_NEIGHBOUR_SHIFT) | (packedData & ~PREV_NEIGHBOUR_MASK);
  }

  void setBothGradients(float grad) {
    xGrad = grad;
    yGrad = grad;
  }
};


class TopBottomEdgeTracer::PrioQueue : public PriorityQueue<uint32_t, PrioQueue> {
 public:
  explicit PrioQueue(Grid<GridNode>& grid) : m_data(grid.data()) {}

  bool higherThan(uint32_t lhs, uint32_t rhs) const { return m_data[lhs].pathCost < m_data[rhs].pathCost; }

  void setIndex(uint32_t gridIdx, size_t heapIdx) { m_data[gridIdx].setHeapIdx(static_cast<uint32_t>(heapIdx)); }

  void reposition(GridNode* node) { PriorityQueue<uint32_t, PrioQueue>::reposition(node->heapIdx()); }

 private:
  GridNode* const m_data;
};


struct TopBottomEdgeTracer::Step {
  Vec2f pt;
  uint32_t prevStepIdx{};
  float pathCost{};
};


template <typename Extractor>
float TopBottomEdgeTracer::interpolatedGridValue(const Grid<GridNode>& grid,
                                                 Extractor extractor,
                                                 const Vec2f pos,
                                                 float defaultValue) {
  const auto xBase = static_cast<float>(std::floor(pos[0]));
  const auto yBase = static_cast<float>(std::floor(pos[1]));
  const auto xBaseI = (int) xBase;
  const auto yBaseI = (int) yBase;

  if ((xBaseI < 0) || (yBaseI < 0) || (xBaseI + 1 >= grid.width()) || (yBaseI + 1 >= grid.height())) {
    return defaultValue;
  }

  const float x = pos[0] - xBase;
  const float y = pos[1] - yBase;
  const float x1 = 1.0f - x;
  const float y1 = 1.0f - y;

  const int stride = grid.stride();
  const GridNode* base = grid.data() + yBaseI * stride + xBaseI;

  return extractor(base[0]) * x1 * y1 + extractor(base[1]) * x * y1 + extractor(base[stride]) * x1 * y
         + extractor(base[stride + 1]) * x * y;
}

void TopBottomEdgeTracer::trace(const imageproc::GrayImage& image,
                                std::pair<QLineF, QLineF> bounds,
                                DistortionModelBuilder& output,
                                const TaskStatus& status,
                                DebugImages* dbg) {
  if ((bounds.first.p1() == bounds.first.p2()) || (bounds.second.p1() == bounds.second.p2())) {
    return;  // Bad bounds.
  }

  GrayImage downscaled;
  QSize downscaledSize(image.size());
  QTransform downscalingXform;

  if (std::max(image.width(), image.height()) < 1500) {
    // Don't downscale - it's already small.
    downscaled = image;
  } else {
    // Proceed with downscaling.
    downscaledSize.scale(1000, 1000, Qt::KeepAspectRatio);
    downscalingXform.scale(double(downscaledSize.width()) / image.width(),
                           double(downscaledSize.height()) / image.height());
    downscaled = scaleToGray(image, downscaledSize);
    if (dbg) {
      dbg->add(downscaled, "downscaled");
    }

    status.throwIfCancelled();

    bounds.first = downscalingXform.map(bounds.first);
    bounds.second = downscalingXform.map(bounds.second);
  }

  // Those -1's are to make sure the endpoints, rounded to integers,
  // will be within the image.
  if (!intersectWithRect(bounds, QRectF(downscaled.rect()).adjusted(0, 0, -1, -1))) {
    return;
  }

  forceSameDirection(bounds);

  const Vec2f avgBoundsDir(calcAvgUnitVector(bounds));
  Grid<GridNode> grid(downscaled.width(), downscaled.height(), /*padding=*/1);
  calcDirectionalDerivative(grid, downscaled, avgBoundsDir);
  if (dbg) {
    dbg->add(visualizeGradient(grid), "gradient");
  }

  status.throwIfCancelled();

  PrioQueue queue(grid);

  // Shortest paths from bounds.first towards bounds.second.
  prepareForShortestPathsFrom(queue, grid, bounds.first);
  const Vec2f dir1stTo2nd(directionFromPointToLine(bounds.first.pointAt(0.5), bounds.second));
  propagateShortestPaths(dir1stTo2nd, queue, grid);
  const std::vector<QPoint> endpoints1(locateBestPathEndpoints(grid, bounds.second));
  if (dbg) {
    dbg->add(visualizePaths(downscaled, grid, bounds, endpoints1), "best_paths_ltr");
  }

  gaussBlurGradient(grid);

  std::vector<std::vector<QPointF>> snakes;
  snakes.reserve(endpoints1.size());

  for (QPoint endpoint : endpoints1) {
    snakes.push_back(pathToSnake(grid, endpoint));
    const Vec2f dir(downTheHillDirection(downscaled.rect(), snakes.back(), avgBoundsDir));
    downTheHillSnake(snakes.back(), grid, dir);
  }
  if (dbg) {
    const QImage background(visualizeBlurredGradient(grid));
    dbg->add(visualizeSnakes(background, snakes, bounds), "down_the_hill_snakes");
  }

  for (std::vector<QPointF>& snake : snakes) {
    const Vec2f dir(-downTheHillDirection(downscaled.rect(), snake, avgBoundsDir));
    upTheHillSnake(snake, grid, dir);
  }
  if (dbg) {
    const QImage background(visualizeGradient(grid));
    dbg->add(visualizeSnakes(background, snakes, bounds), "up_the_hill_snakes");
  }

  // Convert snakes back to the original coordinate system.
  const QTransform upscalingXform(downscalingXform.inverted());
  for (std::vector<QPointF>& snake : snakes) {
    for (QPointF& pt : snake) {
      pt = upscalingXform.map(pt);
    }
    output.addHorizontalCurve(snake);
  }
}  // TopBottomEdgeTracer::trace

bool TopBottomEdgeTracer::intersectWithRect(std::pair<QLineF, QLineF>& bounds, const QRectF& rect) {
  return lineBoundedByRect(bounds.first, rect) && lineBoundedByRect(bounds.second, rect);
}

void TopBottomEdgeTracer::forceSameDirection(std::pair<QLineF, QLineF>& bounds) {
  const QPointF v1(bounds.first.p2() - bounds.first.p1());
  const QPointF v2(bounds.second.p2() - bounds.second.p1());
  if (v1.x() * v2.x() + v1.y() * v2.y() < 0) {
    bounds.second.setPoints(bounds.second.p2(), bounds.second.p1());
  }
}

void TopBottomEdgeTracer::calcDirectionalDerivative(Grid<GridNode>& grid,
                                                    const imageproc::GrayImage& image,
                                                    const Vec2f& direction) {
  assert(grid.padding() == 1);

  const int width = grid.width();
  const int height = grid.height();

  const int gridStride = grid.stride();
  const int imageStride = image.stride();

  const uint8_t* imageLine = image.data();
  GridNode* gridLine = grid.data();

  // This ensures that partial derivatives never go beyond the [-1, 1] range.
  const float scale = 1.0f / (255.0f * 8.0f);

  // We are going to use both GridNode::gradient and GridNode::pathCost
  // to calculate the gradient.

  // Copy image to gradient.
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      gridLine[x].setBothGradients(scale * imageLine[x]);
    }
    imageLine += imageStride;
    gridLine += gridStride;
  }

  // Write border corners.
  gridLine = grid.paddedData();
  gridLine[0].setBothGradients(gridLine[gridStride + 1].xGrad);
  gridLine[gridStride - 1].setBothGradients(gridLine[gridStride * 2 - 2].xGrad);
  gridLine += gridStride * (height + 1);
  gridLine[0].setBothGradients(gridLine[1 - gridStride].xGrad);
  gridLine[gridStride - 1].setBothGradients(gridLine[-2].xGrad);

  // Top border line.
  gridLine = grid.paddedData() + 1;
  for (int x = 0; x < width; ++x) {
    gridLine[0].setBothGradients(gridLine[gridStride].xGrad);
    ++gridLine;
  }

  // Bottom border line.
  gridLine = grid.paddedData() + gridStride * (height + 1) + 1;
  for (int x = 0; x < width; ++x) {
    gridLine[0].setBothGradients(gridLine[-gridStride].xGrad);
    ++gridLine;
  }
  // Left and right border lines.
  gridLine = grid.paddedData() + gridStride;
  for (int y = 0; y < height; ++y) {
    gridLine[0].setBothGradients(gridLine[1].xGrad);
    gridLine[gridStride - 1].setBothGradients(gridLine[gridStride - 2].xGrad);
    gridLine += gridStride;
  }

  horizontalSobelInPlace(grid);
  verticalSobelInPlace(grid);
  // From horizontal and vertical gradients, calculate the directional one.
  gridLine = grid.data();
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      const Vec2f gradVec(gridLine[x].xGrad, gridLine[x].yGrad);
      gridLine[x].dirDeriv = gradVec.dot(direction);
      assert(std::fabs(gridLine[x].dirDeriv) <= 1.0);
    }

    gridLine += gridStride;
  }
}  // TopBottomEdgeTracer::calcDirectionalDerivative

void TopBottomEdgeTracer::horizontalSobelInPlace(Grid<GridNode>& grid) {
  assert(grid.padding() == 1);

  const int width = grid.width();
  const int height = grid.height();
  const int gridStride = grid.stride();

  // Do a vertical pass.
  for (int x = -1; x < width + 1; ++x) {
    GridNode* pGrid = grid.data() + x;
    float prev = pGrid[-gridStride].xGrad;
    for (int y = 0; y < height; ++y) {
      const float cur = pGrid->xGrad;
      pGrid->xGrad = prev + cur + cur + pGrid[gridStride].xGrad;
      prev = cur;
      pGrid += gridStride;
    }
  }

  // Do a horizontal pass and write results.
  GridNode* gridLine = grid.data();
  for (int y = 0; y < height; ++y) {
    float prev = gridLine[-1].xGrad;
    for (int x = 0; x < width; ++x) {
      float cur = gridLine[x].xGrad;
      gridLine[x].xGrad = gridLine[x + 1].xGrad - prev;
      prev = cur;
    }
    gridLine += gridStride;
  }
}

void TopBottomEdgeTracer::verticalSobelInPlace(Grid<GridNode>& grid) {
  assert(grid.padding() == 1);

  const int width = grid.width();
  const int height = grid.height();
  const int gridStride = grid.stride();
  // Do a horizontal pass.
  GridNode* gridLine = grid.paddedData() + 1;
  for (int y = 0; y < height + 2; ++y) {
    float prev = gridLine[-1].yGrad;
    for (int x = 0; x < width; ++x) {
      float cur = gridLine[x].yGrad;
      gridLine[x].yGrad = prev + cur + cur + gridLine[x + 1].yGrad;
      prev = cur;
    }
    gridLine += gridStride;
  }

  // Do a vertical pass and write resuts.
  for (int x = 0; x < width; ++x) {
    GridNode* pGrid = grid.data() + x;
    float prev = pGrid[-gridStride].yGrad;
    for (int y = 0; y < height; ++y) {
      const float cur = pGrid->yGrad;
      pGrid->yGrad = pGrid[gridStride].yGrad - prev;
      prev = cur;
      pGrid += gridStride;
    }
  }
}

Vec2f TopBottomEdgeTracer::calcAvgUnitVector(const std::pair<QLineF, QLineF>& bounds) {
  Vec2f v1(bounds.first.p2() - bounds.first.p1());
  v1 /= std::sqrt(v1.squaredNorm());

  Vec2f v2(bounds.second.p2() - bounds.second.p1());
  v2 /= std::sqrt(v2.squaredNorm());

  Vec2f v3(v1 + v2);
  v3 /= std::sqrt(v3.squaredNorm());

  return v3;
}

Vec2f TopBottomEdgeTracer::directionFromPointToLine(const QPointF& pt, const QLineF& line) {
  Vec2f vec(ToLineProjector(line).projectionVector(pt));
  const float sqlen = vec.squaredNorm();
  if (sqlen > 1e-5) {
    vec /= std::sqrt(sqlen);
  }

  return vec;
}

void TopBottomEdgeTracer::prepareForShortestPathsFrom(PrioQueue& queue, Grid<GridNode>& grid, const QLineF& from) {
  GridNode padding_node{};
  padding_node.setupForPadding();
  grid.initPadding(padding_node);

  const int width = grid.width();
  const int height = grid.height();
  const int stride = grid.stride();
  GridNode* const data = grid.data();

  GridNode* line = grid.data();
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      GridNode* node = line + x;
      node->setupForInterior();
      // This doesn't modify dirDeriv, which is why
      // we can't use grid.initInterior().
    }
    line += stride;
  }

  GridLineTraverser traverser(from);
  while (traverser.hasNext()) {
    const QPoint pt(traverser.next());

    // intersectWithRect() ensures that.
    assert(pt.x() >= 0 && pt.y() >= 0 && pt.x() < width && pt.y() < height);

    const int offset = pt.y() * stride + pt.x();
    data[offset].pathCost = 0;
    queue.push(offset);
  }
}

void TopBottomEdgeTracer::propagateShortestPaths(const Vec2f& direction, PrioQueue& queue, Grid<GridNode>& grid) {
  GridNode* const data = grid.data();

  int nextNbhOffsets[8];
  int prevNbhIndexes[8];
  const int numNeighbours = initNeighbours(nextNbhOffsets, prevNbhIndexes, grid.stride(), direction);

  while (!queue.empty()) {
    const int gridIdx = queue.front();
    GridNode* node = data + gridIdx;
    assert(node->pathCost >= 0);
    queue.pop();
    node->setHeapIdx(GridNode::INVALID_HEAP_IDX);

    for (int i = 0; i < numNeighbours; ++i) {
      const int nbhGridIdx = gridIdx + nextNbhOffsets[i];
      GridNode* nbhNode = data + nbhGridIdx;

      assert(std::fabs(node->dirDeriv) <= 1.0);
      const float new_cost
          = std::max<float>(node->pathCost, static_cast<const float&>(1.0f - std::fabs(node->dirDeriv)));
      if (new_cost < nbhNode->pathCost) {
        nbhNode->pathCost = new_cost;
        nbhNode->setPrevNeighbourIdx(prevNbhIndexes[i]);
        if (nbhNode->heapIdx() == GridNode::INVALID_HEAP_IDX) {
          queue.push(nbhGridIdx);
        } else {
          queue.reposition(nbhNode);
        }
      }
    }
  }
}  // TopBottomEdgeTracer::propagateShortestPaths

int TopBottomEdgeTracer::initNeighbours(int* nextNbhOffsets, int* prevNbhIndexes, int stride, const Vec2f& direction) {
  const int candidate_offsets[] = {-stride - 1, -stride, -stride + 1, -1, 1, stride - 1, stride, stride + 1};

  const float candidate_vectors[8][2] = {{-1.0f, -1.0f}, {0.0f, -1.0f}, {1.0f, -1.0f}, {-1.0f, 0.0f},
                                         {1.0f, 0.0f},   {-1.0f, 1.0f}, {0.0f, 1.0f},  {1.0f, 1.0f}};

  static const int opposite_nbh_map[] = {7, 6, 5, 4, 3, 2, 1, 0};

  int outIdx = 0;
  for (int i = 0; i < 8; ++i) {
    const Vec2f vec(candidate_vectors[i][0], candidate_vectors[i][1]);
    if (vec.dot(direction) > 0) {
      nextNbhOffsets[outIdx] = candidate_offsets[i];
      prevNbhIndexes[outIdx] = opposite_nbh_map[i];
      ++outIdx;
    }
  }

  return outIdx;
}  // TopBottomEdgeTracer::initNeighbours

namespace {
struct Path {
  QPoint pt;
  float cost;

  Path(QPoint pt, float cost) : pt(pt), cost(cost) {}
};
}  // namespace

std::vector<QPoint> TopBottomEdgeTracer::locateBestPathEndpoints(const Grid<GridNode>& grid, const QLineF& line) {
  const int width = grid.width();
  const int height = grid.height();
  const int stride = grid.stride();
  const GridNode* const data = grid.data();

  const size_t numBestPaths = 2;  // Take N best paths.
  const int minSqdist = 100 * 100;
  std::vector<Path> bestPaths;

  GridLineTraverser traverser(line);
  while (traverser.hasNext()) {
    const QPoint pt(traverser.next());

    // intersectWithRect() ensures that.
    assert(pt.x() >= 0 && pt.y() >= 0 && pt.x() < width && pt.y() < height);

    const uint32_t offset = pt.y() * stride + pt.x();
    const GridNode* node = data + offset;

    // Find the closest path.
    Path* closestPath = nullptr;
    int closestSqdist = std::numeric_limits<int>::max();
    for (Path& path : bestPaths) {
      const QPoint delta(path.pt - pt);
      const int sqdist = delta.x() * delta.x() + delta.y() * delta.y();
      if (sqdist < closestSqdist) {
        closestPath = &path;
        closestSqdist = sqdist;
      }
    }

    if (closestSqdist < minSqdist) {
      // That's too close.
      if (node->pathCost < closestPath->cost) {
        closestPath->pt = pt;
        closestPath->cost = node->pathCost;
      }
      continue;
    }

    if (bestPaths.size() < numBestPaths) {
      bestPaths.emplace_back(pt, node->pathCost);
    } else {
      // Find the one to kick out (if any).
      for (Path& path : bestPaths) {
        if (node->pathCost < path.cost) {
          path = Path(pt, node->pathCost);
          break;
        }
      }
    }
  }

  std::vector<QPoint> bestEndpoints;

  for (const Path& path : bestPaths) {
    if (path.cost < 0.95f) {
      bestEndpoints.push_back(path.pt);
    }
  }

  return bestEndpoints;
}  // TopBottomEdgeTracer::locateBestPathEndpoints

std::vector<QPoint> TopBottomEdgeTracer::tracePathFromEndpoint(const Grid<GridNode>& grid, const QPoint& endpoint) {
  static const int dx[8] = {-1, 0, 1, -1, 1, -1, 0, 1};
  static const int dy[8] = {-1, -1, -1, 0, 0, 1, 1, 1};

  const int stride = grid.stride();
  const int grid_offsets[8] = {-stride - 1, -stride, -stride + 1, -1, +1, +stride - 1, +stride, +stride + 1};

  const GridNode* const data = grid.data();
  std::vector<QPoint> path;

  QPoint pt(endpoint);
  int gridOffset = pt.x() + pt.y() * stride;
  while (true) {
    path.push_back(pt);

    const GridNode* node = data + gridOffset;
    if (!node->hasPathContinuation()) {
      break;
    }

    const int nbhIdx = node->prevNeighbourIdx();
    gridOffset += grid_offsets[nbhIdx];
    pt += QPoint(dx[nbhIdx], dy[nbhIdx]);
  }

  return path;
}  // TopBottomEdgeTracer::tracePathFromEndpoint

std::vector<QPointF> TopBottomEdgeTracer::pathToSnake(const Grid<GridNode>& grid, const QPoint& endpoint) {
  const int maxDist = 15;  // Maximum distance between two snake knots.
  const int maxDistSq = maxDist * maxDist;
  const int halfMaxDist = maxDist / 2;
  const int halfMaxDistSq = halfMaxDist * halfMaxDist;

  static const int dx[8] = {-1, 0, 1, -1, 1, -1, 0, 1};
  static const int dy[8] = {-1, -1, -1, 0, 0, 1, 1, 1};

  const int stride = grid.stride();
  const int grid_offsets[8] = {-stride - 1, -stride, -stride + 1, -1, +1, +stride - 1, +stride, +stride + 1};

  const GridNode* const data = grid.data();
  std::vector<QPointF> snake;
  snake.emplace_back(endpoint);
  QPoint snakeTail(endpoint);

  QPoint pt(endpoint);
  int gridOffset = pt.x() + pt.y() * stride;
  while (true) {
    const QPoint delta(pt - snakeTail);
    const int sqdist = delta.x() * delta.x() + delta.y() * delta.y();

    const GridNode* node = data + gridOffset;
    if (!node->hasPathContinuation()) {
      if (sqdist >= halfMaxDistSq) {
        snake.emplace_back(pt);
        snakeTail = pt;
      }
      break;
    }

    if (sqdist >= maxDistSq) {
      snake.emplace_back(pt);
      snakeTail = pt;
    }

    const int nbhIdx = node->prevNeighbourIdx();
    gridOffset += grid_offsets[nbhIdx];
    pt += QPoint(dx[nbhIdx], dy[nbhIdx]);
  }

  return snake;
}  // TopBottomEdgeTracer::pathToSnake

void TopBottomEdgeTracer::gaussBlurGradient(Grid<GridNode>& grid) {
  using namespace boost::lambda;

  gaussBlurGeneric(QSize(grid.width(), grid.height()), 2.0f, 2.0f, grid.data(), grid.stride(),
                   bind(&GridNode::absDirDeriv, _1), grid.data(), grid.stride(), bind(&GridNode::blurred, _1) = _2);
}

Vec2f TopBottomEdgeTracer::downTheHillDirection(const QRectF& pageRect,
                                                const std::vector<QPointF>& snake,
                                                const Vec2f& boundsDir) {
  assert(!snake.empty());

  // Take the centroid of a snake.
  QPointF centroid;
  for (const QPointF& pt : snake) {
    centroid += pt;
  }
  centroid /= snake.size();

  QLineF line(centroid, centroid + boundsDir);
  lineBoundedByRect(line, pageRect);
  // The downhill direction is the direction *inside* the page.
  const Vec2d v1(line.p1() - centroid);
  const Vec2d v2(line.p2() - centroid);
  if (v1.squaredNorm() > v2.squaredNorm()) {
    return v1;
  } else {
    return v2;
  }
}

void TopBottomEdgeTracer::downTheHillSnake(std::vector<QPointF>& snake, const Grid<GridNode>& grid, const Vec2f dir) {
  using namespace boost::lambda;

  const size_t numNodes = snake.size();
  if (numNodes <= 1) {
    return;
  }

  float avgDist = 0;
  for (size_t i = 1; i < numNodes; ++i) {
    const Vec2f vec(snake[i] - snake[i - 1]);
    avgDist += std::sqrt(vec.squaredNorm());
  }
  avgDist /= numNodes - 1;

  std::vector<Step> stepStorage;

  Vec2f displacements[9];
  const int numDisplacements = initDisplacementVectors(displacements, dir);

  const float elasticityWeight = 0.6f;
  const float bendingWeight = 8.0f;
  const float externalWeight = 0.4f;

  const float segmentDistThreshold = 1;

  for (int iteration = 0; iteration < 40; ++iteration) {
    stepStorage.clear();

    std::vector<uint32_t> paths;
    std::vector<uint32_t> newPaths;

    for (size_t nodeIdx = 0; nodeIdx < numNodes; ++nodeIdx) {
      const Vec2f pt(snake[nodeIdx]);
      const float curExternalEnergy = interpolatedGridValue(grid, bind<float>(&GridNode::blurred, _1), pt, 1000);

      for (int displacementIdx = 0; displacementIdx < numDisplacements; ++displacementIdx) {
        Step step;
        step.prevStepIdx = ~uint32_t(0);
        step.pt = pt + displacements[displacementIdx];
        step.pathCost = 0;

        const float adjusted_external_energy
            = interpolatedGridValue(grid, bind<float>(&GridNode::blurred, _1), step.pt, 1000);
        if (displacementIdx == 0) {
          step.pathCost += 100;
        } else if (curExternalEnergy < 0.01) {
          if (curExternalEnergy - adjusted_external_energy < 0.01f) {
            continue;
          }
        }

        step.pathCost += externalWeight * adjusted_external_energy;

        float bestCost = NumericTraits<float>::max();
        uint32_t bestPrevStepIdx = step.prevStepIdx;

        for (uint32_t prevStepIdx : paths) {
          const Step& prevStep = stepStorage[prevStepIdx];
          float cost = prevStep.pathCost + step.pathCost;

          const Vec2f vec(step.pt - prevStep.pt);
          const auto vecLen = static_cast<float>(std::sqrt(vec.squaredNorm()));
          if (vecLen < segmentDistThreshold) {
            cost += 1000;
          }

          // Elasticity.
          const auto distDiff = std::fabs(avgDist - vecLen);
          cost += elasticityWeight * (distDiff / avgDist);
          // Bending energy.
          if ((prevStep.prevStepIdx != ~uint32_t(0)) && (vecLen >= segmentDistThreshold)) {
            const Step& prevPrevStep = stepStorage[prevStep.prevStepIdx];
            Vec2f prevNormal(prevStep.pt - prevPrevStep.pt);
            std::swap(prevNormal[0], prevNormal[1]);
            prevNormal[0] = -prevNormal[0];
            const auto prevNormalLen = static_cast<float>(std::sqrt(prevNormal.squaredNorm()));
            if (prevNormalLen < segmentDistThreshold) {
              cost += 1000;
            } else {
              const float cos = vec.dot(prevNormal) / (vecLen * prevNormalLen);
              // cost += 0.7 * std::fabs(cos);
              cost += bendingWeight * cos * cos;
            }
          }

          assert(cost < NumericTraits<float>::max());

          if (cost < bestCost) {
            bestCost = cost;
            bestPrevStepIdx = prevStepIdx;
          }
        }

        step.prevStepIdx = bestPrevStepIdx;
        if (bestPrevStepIdx != ~uint32_t(0)) {
          step.pathCost = bestCost;
        }

        newPaths.push_back(static_cast<unsigned int&&>(stepStorage.size()));
        stepStorage.push_back(step);
      }
      assert(!newPaths.empty());
      paths.swap(newPaths);
      newPaths.clear();
    }

    uint32_t bestPathIdx = ~uint32_t(0);
    float bestCost = NumericTraits<float>::max();
    for (uint32_t lastStepIdx : paths) {
      const Step& step = stepStorage[lastStepIdx];
      if (step.pathCost < bestCost) {
        bestCost = step.pathCost;
        bestPathIdx = lastStepIdx;
      }
    }
    // Having found the best path, convert it back to a snake.
    snake.clear();
    uint32_t stepIdx = bestPathIdx;
    while (stepIdx != ~uint32_t(0)) {
      const Step& step = stepStorage[stepIdx];
      snake.push_back(step.pt);
      stepIdx = step.prevStepIdx;
    }
    assert(numNodes == snake.size());
  }
}  // TopBottomEdgeTracer::downTheHillSnake

void TopBottomEdgeTracer::upTheHillSnake(std::vector<QPointF>& snake, const Grid<GridNode>& grid, const Vec2f dir) {
  using namespace boost::lambda;

  const size_t numNodes = snake.size();
  if (numNodes <= 1) {
    return;
  }

  float avgDist = 0;
  for (size_t i = 1; i < numNodes; ++i) {
    const Vec2f vec(snake[i] - snake[i - 1]);
    avgDist += std::sqrt(vec.squaredNorm());
  }
  avgDist /= numNodes - 1;

  std::vector<Step> stepStorage;

  Vec2f displacements[9];
  const int numDisplacements = initDisplacementVectors(displacements, dir);
  for (int i = 0; i < numDisplacements; ++i) {
    // We need more accuracy here.
    displacements[i] *= 0.5f;
  }

  const float elasticityWeight = 0.6f;
  const float bendingWeight = 3.0f;
  const float externalWeight = 2.0f;

  const float segmentDistThreshold = 1;

  for (int iteration = 0; iteration < 40; ++iteration) {
    stepStorage.clear();

    std::vector<uint32_t> paths;
    std::vector<uint32_t> newPaths;

    for (size_t nodeIdx = 0; nodeIdx < numNodes; ++nodeIdx) {
      const Vec2f pt(snake[nodeIdx]);
      const float curExternalEnergy = -interpolatedGridValue(grid, bind<float>(&GridNode::absDirDeriv, _1), pt, 1000);

      for (int displacementIdx = 0; displacementIdx < numDisplacements; ++displacementIdx) {
        Step step;
        step.prevStepIdx = ~uint32_t(0);
        step.pt = pt + displacements[displacementIdx];
        step.pathCost = 0;

        const float adjusted_external_energy
            = -interpolatedGridValue(grid, bind<float>(&GridNode::absDirDeriv, _1), step.pt, 1000);
        if ((displacementIdx == 0) && (adjusted_external_energy > -0.02)) {
          // Discorage staying on the spot if the gradient magnitude is too
          // small at that point.
          step.pathCost += 100;
        }

        step.pathCost += externalWeight * adjusted_external_energy;

        float bestCost = NumericTraits<float>::max();
        uint32_t bestPrevStepIdx = step.prevStepIdx;

        for (uint32_t prevStepIdx : paths) {
          const Step& prevStep = stepStorage[prevStepIdx];
          float cost = prevStep.pathCost + step.pathCost;

          const Vec2f vec(step.pt - prevStep.pt);
          const auto vecLen = static_cast<float>(std::sqrt(vec.squaredNorm()));
          if (vecLen < segmentDistThreshold) {
            cost += 1000;
          }

          // Elasticity.
          const auto distDiff = std::fabs(avgDist - vecLen);
          cost += elasticityWeight * (distDiff / avgDist);
          // Bending energy.
          if ((prevStep.prevStepIdx != ~uint32_t(0)) && (vecLen >= segmentDistThreshold)) {
            const Step& prevPrevStep = stepStorage[prevStep.prevStepIdx];
            Vec2f prevNormal(prevStep.pt - prevPrevStep.pt);
            std::swap(prevNormal[0], prevNormal[1]);
            prevNormal[0] = -prevNormal[0];
            const auto prevNormalLen = static_cast<float>(std::sqrt(prevNormal.squaredNorm()));
            if (prevNormalLen < segmentDistThreshold) {
              cost += 1000;
            } else {
              const float cos = vec.dot(prevNormal) / (vecLen * prevNormalLen);
              // cost += 0.7 * std::fabs(cos);
              cost += bendingWeight * cos * cos;
            }
          }

          assert(cost < NumericTraits<float>::max());

          if (cost < bestCost) {
            bestCost = cost;
            bestPrevStepIdx = prevStepIdx;
          }
        }

        step.prevStepIdx = bestPrevStepIdx;
        if (bestPrevStepIdx != ~uint32_t(0)) {
          step.pathCost = bestCost;
        }

        newPaths.push_back(static_cast<unsigned int&&>(stepStorage.size()));
        stepStorage.push_back(step);
      }
      assert(!newPaths.empty());
      paths.swap(newPaths);
      newPaths.clear();
    }

    uint32_t bestPathIdx = ~uint32_t(0);
    float bestCost = NumericTraits<float>::max();
    for (uint32_t lastStepIdx : paths) {
      const Step& step = stepStorage[lastStepIdx];
      if (step.pathCost < bestCost) {
        bestCost = step.pathCost;
        bestPathIdx = lastStepIdx;
      }
    }
    // Having found the best path, convert it back to a snake.
    snake.clear();
    uint32_t stepIdx = bestPathIdx;
    while (stepIdx != ~uint32_t(0)) {
      const Step& step = stepStorage[stepIdx];
      snake.push_back(step.pt);
      stepIdx = step.prevStepIdx;
    }
    assert(numNodes == snake.size());
  }
}  // TopBottomEdgeTracer::upTheHillSnake

int TopBottomEdgeTracer::initDisplacementVectors(Vec2f vectors[], Vec2f validDirection) {
  int outIdx = 0;
  // This one must always be present, and must be first, as we want to prefer it
  // over another one with exactly the same score.
  vectors[outIdx++] = Vec2f(0, 0);

  static const float dx[] = {-1, 0, 1, -1, 1, -1, 0, 1};

  static const float dy[] = {-1, -1, -1, 0, 0, 1, 1, 1};

  for (int i = 0; i < 8; ++i) {
    const Vec2f vec(dx[i], dy[i]);
    if (vec.dot(validDirection) > 0) {
      vectors[outIdx++] = vec;
    }
  }

  return outIdx;
}

QImage TopBottomEdgeTracer::visualizeGradient(const Grid<GridNode>& grid, const QImage* background) {
  const int width = grid.width();
  const int height = grid.height();
  const int gridStride = grid.stride();
  // First let's find the maximum and minimum values.
  float minValue = NumericTraits<float>::max();
  float maxValue = NumericTraits<float>::min();

  const GridNode* gridLine = grid.data();
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      const float value = gridLine[x].dirDeriv;
      if (value < minValue) {
        minValue = value;
      } else if (value > maxValue) {
        maxValue = value;
      }
    }
    gridLine += gridStride;
  }

  float scale = std::max(maxValue, -minValue);
  if (scale > std::numeric_limits<float>::epsilon()) {
    scale = 255.0f / scale;
  }

  QImage overlay(width, height, QImage::Format_ARGB32_Premultiplied);
  auto* overlayLine = (uint32_t*) overlay.bits();
  const int overlayStride = overlay.bytesPerLine() / 4;

  gridLine = grid.data();
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      const float value = gridLine[x].dirDeriv * scale;
      const int magnitude = qBound(0, static_cast<const int&>(std::round(std::fabs(value))), 255);
      if (value > 0) {
        // Red for positive gradients which indicate bottom edges.
        overlayLine[x] = qRgba(magnitude, 0, 0, magnitude);
      } else {
        overlayLine[x] = qRgba(0, 0, magnitude, magnitude);
      }
    }
    gridLine += gridStride;
    overlayLine += overlayStride;
  }

  QImage canvas;
  if (background) {
    canvas = background->convertToFormat(QImage::Format_ARGB32_Premultiplied);
  } else {
    canvas = QImage(width, height, QImage::Format_ARGB32_Premultiplied);
    canvas.fill(0xffffffff);  // Opaque white.
  }

  QPainter painter(&canvas);
  painter.drawImage(0, 0, overlay);

  return canvas;
}  // TopBottomEdgeTracer::visualizeGradient

QImage TopBottomEdgeTracer::visualizeBlurredGradient(const Grid<GridNode>& grid) {
  const int width = grid.width();
  const int height = grid.height();
  const int gridStride = grid.stride();
  // First let's find the maximum and minimum values.
  float minValue = NumericTraits<float>::max();
  float maxValue = NumericTraits<float>::min();

  const GridNode* gridLine = grid.data();
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      const float value = gridLine[x].blurred;
      if (value < minValue) {
        minValue = value;
      } else if (value > maxValue) {
        maxValue = value;
      }
    }
    gridLine += gridStride;
  }

  float scale = std::max(maxValue, -minValue);
  if (scale > std::numeric_limits<float>::epsilon()) {
    scale = 255.0f / scale;
  }

  QImage overlay(width, height, QImage::Format_ARGB32_Premultiplied);
  auto* overlayLine = (uint32_t*) overlay.bits();
  const int overlayStride = overlay.bytesPerLine() / 4;

  gridLine = grid.data();
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      const float value = gridLine[x].blurred * scale;
      const int magnitude = qBound(0, static_cast<const int&>(std::round(std::fabs(value))), 255);
      overlayLine[x] = qRgba(magnitude, 0, 0, magnitude);
    }
    gridLine += gridStride;
    overlayLine += overlayStride;
  }

  QImage canvas(grid.width(), grid.height(), QImage::Format_ARGB32_Premultiplied);
  canvas.fill(0xffffffff);  // Opaque white.
  QPainter painter(&canvas);
  painter.drawImage(0, 0, overlay);

  return canvas;
}  // TopBottomEdgeTracer::visualizeBlurredGradient

QImage TopBottomEdgeTracer::visualizePaths(const QImage& background,
                                           const Grid<GridNode>& grid,
                                           const std::pair<QLineF, QLineF>& bounds,
                                           const std::vector<QPoint>& pathEndpoints) {
  QImage canvas(background.convertToFormat(QImage::Format_RGB32));
  auto* const canvasData = (uint32_t*) canvas.bits();
  const int canvasStride = canvas.bytesPerLine() / 4;

  const int width = grid.width();
  const int height = grid.height();
  const int gridStride = grid.stride();
  const GridNode* const gridData = grid.data();

  const int nbh_canvas_offsets[8] = {-canvasStride - 1, -canvasStride, -canvasStride + 1, -1, +1,
                                     +canvasStride - 1, +canvasStride, +canvasStride + 1};
  const int nbh_grid_offsets[8]
      = {-gridStride - 1, -gridStride, -gridStride + 1, -1, +1, +gridStride - 1, +gridStride, +gridStride + 1};

  for (const QPoint pathEndpoint : pathEndpoints) {
    int gridOffset = pathEndpoint.x() + pathEndpoint.y() * gridStride;
    int canvasOffset = pathEndpoint.x() + pathEndpoint.y() * canvasStride;
    while (true) {
      const GridNode* node = gridData + gridOffset;
      canvasData[canvasOffset] = 0x00ff0000;
      if (!node->hasPathContinuation()) {
        break;
      }

      const int nbhIdx = node->prevNeighbourIdx();
      gridOffset += nbh_grid_offsets[nbhIdx];
      canvasOffset += nbh_canvas_offsets[nbhIdx];
    }
  }

  QPainter painter(&canvas);
  painter.setRenderHint(QPainter::Antialiasing);
  QPen pen(Qt::blue);
  pen.setWidthF(1.0);
  painter.setPen(pen);
  painter.drawLine(bounds.first);
  painter.drawLine(bounds.second);

  return canvas;
}  // TopBottomEdgeTracer::visualizePaths

QImage TopBottomEdgeTracer::visualizePaths(const QImage& background,
                                           const std::vector<std::vector<QPoint>>& paths,
                                           const std::pair<QLineF, QLineF>& bounds) {
  QImage canvas(background.convertToFormat(QImage::Format_RGB32));
  auto* const canvasData = (uint32_t*) canvas.bits();
  const int canvasStride = canvas.bytesPerLine() / 4;

  for (const std::vector<QPoint>& path : paths) {
    for (QPoint pt : path) {
      canvasData[pt.x() + pt.y() * canvasStride] = 0x00ff0000;
    }
  }

  QPainter painter(&canvas);
  painter.setRenderHint(QPainter::Antialiasing);
  QPen pen(Qt::blue);
  pen.setWidthF(1.0);
  painter.setPen(pen);
  painter.drawLine(bounds.first);
  painter.drawLine(bounds.second);

  return canvas;
}

QImage TopBottomEdgeTracer::visualizeSnakes(const QImage& background,
                                            const std::vector<std::vector<QPointF>>& snakes,
                                            const std::pair<QLineF, QLineF>& bounds) {
  QImage canvas(background.convertToFormat(QImage::Format_ARGB32_Premultiplied));
  QPainter painter(&canvas);
  painter.setRenderHint(QPainter::Antialiasing);

  QPen snakePen(QColor(0, 255, 0));
  snakePen.setWidthF(1.5);

  QBrush knotBrush(QColor(255, 255, 0, 180));
  painter.setBrush(knotBrush);

  QRectF knotRect(0, 0, 7, 7);

  for (const std::vector<QPointF>& snake : snakes) {
    if (snake.empty()) {
      continue;
    }

    painter.setPen(snakePen);
    painter.drawPolyline(&snake[0], static_cast<int>(snake.size()));
    painter.setPen(Qt::NoPen);
    for (const QPointF& knot : snake) {
      knotRect.moveCenter(knot);
      painter.drawEllipse(knotRect);
    }
  }

  QPen boundsPen(Qt::blue);
  boundsPen.setWidthF(1.5);
  painter.setPen(boundsPen);
  painter.drawLine(bounds.first);
  painter.drawLine(bounds.second);

  return canvas;
}  // TopBottomEdgeTracer::visualizeSnakes

QImage TopBottomEdgeTracer::visualizePolylines(const QImage& background,
                                               const std::list<std::vector<QPointF>>& polylines,
                                               const std::pair<QLineF, QLineF>& bounds) {
  QImage canvas(background.convertToFormat(QImage::Format_ARGB32_Premultiplied));
  QPainter painter(&canvas);
  painter.setRenderHint(QPainter::Antialiasing);

  QPen polylinePen(QColor(255, 0, 0, 100));
  polylinePen.setWidthF(4.0);
  painter.setPen(polylinePen);

  for (const std::vector<QPointF>& polyline : polylines) {
    if (polyline.empty()) {
      continue;
    }

    painter.drawPolyline(&polyline[0], static_cast<int>(polyline.size()));
  }

  QPen boundsPen(Qt::blue);
  boundsPen.setWidthF(1.5);
  painter.setPen(boundsPen);
  painter.drawLine(bounds.first);
  painter.drawLine(bounds.second);

  return canvas;
}
}  // namespace dewarping