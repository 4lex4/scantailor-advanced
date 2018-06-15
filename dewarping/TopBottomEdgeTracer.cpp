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

#include "TopBottomEdgeTracer.h"
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
#include "imageproc/Constants.h"
#include "imageproc/GaussBlur.h"
#include "imageproc/GrayImage.h"
#include "imageproc/Scale.h"

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
  explicit PrioQueue(Grid<GridNode>& grid) : m_pData(grid.data()) {}

  bool higherThan(uint32_t lhs, uint32_t rhs) const { return m_pData[lhs].pathCost < m_pData[rhs].pathCost; }

  void setIndex(uint32_t grid_idx, size_t heap_idx) { m_pData[grid_idx].setHeapIdx(static_cast<uint32_t>(heap_idx)); }

  void reposition(GridNode* node) { PriorityQueue<uint32_t, PrioQueue>::reposition(node->heapIdx()); }

 private:
  GridNode* const m_pData;
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
                                                 float default_value) {
  const auto x_base = static_cast<const float>(std::floor(pos[0]));
  const auto y_base = static_cast<const float>(std::floor(pos[1]));
  const auto x_base_i = (int) x_base;
  const auto y_base_i = (int) y_base;

  if ((x_base_i < 0) || (y_base_i < 0) || (x_base_i + 1 >= grid.width()) || (y_base_i + 1 >= grid.height())) {
    return default_value;
  }

  const float x = pos[0] - x_base;
  const float y = pos[1] - y_base;
  const float x1 = 1.0f - x;
  const float y1 = 1.0f - y;

  const int stride = grid.stride();
  const GridNode* base = grid.data() + y_base_i * stride + x_base_i;

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
  QSize downscaled_size(image.size());
  QTransform downscaling_xform;

  if (std::max(image.width(), image.height()) < 1500) {
    // Don't downscale - it's already small.
    downscaled = image;
  } else {
    // Proceed with downscaling.
    downscaled_size.scale(1000, 1000, Qt::KeepAspectRatio);
    downscaling_xform.scale(double(downscaled_size.width()) / image.width(),
                            double(downscaled_size.height()) / image.height());
    downscaled = scaleToGray(image, downscaled_size);
    if (dbg) {
      dbg->add(downscaled, "downscaled");
    }

    status.throwIfCancelled();

    bounds.first = downscaling_xform.map(bounds.first);
    bounds.second = downscaling_xform.map(bounds.second);
  }

  // Those -1's are to make sure the endpoints, rounded to integers,
  // will be within the image.
  if (!intersectWithRect(bounds, QRectF(downscaled.rect()).adjusted(0, 0, -1, -1))) {
    return;
  }

  forceSameDirection(bounds);

  const Vec2f avg_bounds_dir(calcAvgUnitVector(bounds));
  Grid<GridNode> grid(downscaled.width(), downscaled.height(), /*padding=*/1);
  calcDirectionalDerivative(grid, downscaled, avg_bounds_dir);
  if (dbg) {
    dbg->add(visualizeGradient(grid), "gradient");
  }

  status.throwIfCancelled();

  PrioQueue queue(grid);

  // Shortest paths from bounds.first towards bounds.second.
  prepareForShortestPathsFrom(queue, grid, bounds.first);
  const Vec2f dir_1st_to_2nd(directionFromPointToLine(bounds.first.pointAt(0.5), bounds.second));
  propagateShortestPaths(dir_1st_to_2nd, queue, grid);
  const std::vector<QPoint> endpoints1(locateBestPathEndpoints(grid, bounds.second));
  if (dbg) {
    dbg->add(visualizePaths(downscaled, grid, bounds, endpoints1), "best_paths_ltr");
  }

  gaussBlurGradient(grid);

  std::vector<std::vector<QPointF>> snakes;
  snakes.reserve(endpoints1.size());

  for (QPoint endpoint : endpoints1) {
    snakes.push_back(pathToSnake(grid, endpoint));
    const Vec2f dir(downTheHillDirection(downscaled.rect(), snakes.back(), avg_bounds_dir));
    downTheHillSnake(snakes.back(), grid, dir);
  }
  if (dbg) {
    const QImage background(visualizeBlurredGradient(grid));
    dbg->add(visualizeSnakes(background, snakes, bounds), "down_the_hill_snakes");
  }

  for (std::vector<QPointF>& snake : snakes) {
    const Vec2f dir(-downTheHillDirection(downscaled.rect(), snake, avg_bounds_dir));
    upTheHillSnake(snake, grid, dir);
  }
  if (dbg) {
    const QImage background(visualizeGradient(grid));
    dbg->add(visualizeSnakes(background, snakes, bounds), "up_the_hill_snakes");
  }

  // Convert snakes back to the original coordinate system.
  const QTransform upscaling_xform(downscaling_xform.inverted());
  for (std::vector<QPointF>& snake : snakes) {
    for (QPointF& pt : snake) {
      pt = upscaling_xform.map(pt);
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

  const int grid_stride = grid.stride();
  const int image_stride = image.stride();

  const uint8_t* image_line = image.data();
  GridNode* grid_line = grid.data();

  // This ensures that partial derivatives never go beyond the [-1, 1] range.
  const float scale = 1.0f / (255.0f * 8.0f);

  // We are going to use both GridNode::gradient and GridNode::pathCost
  // to calculate the gradient.

  // Copy image to gradient.
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      grid_line[x].setBothGradients(scale * image_line[x]);
    }
    image_line += image_stride;
    grid_line += grid_stride;
  }

  // Write border corners.
  grid_line = grid.paddedData();
  grid_line[0].setBothGradients(grid_line[grid_stride + 1].xGrad);
  grid_line[grid_stride - 1].setBothGradients(grid_line[grid_stride * 2 - 2].xGrad);
  grid_line += grid_stride * (height + 1);
  grid_line[0].setBothGradients(grid_line[1 - grid_stride].xGrad);
  grid_line[grid_stride - 1].setBothGradients(grid_line[-2].xGrad);

  // Top border line.
  grid_line = grid.paddedData() + 1;
  for (int x = 0; x < width; ++x) {
    grid_line[0].setBothGradients(grid_line[grid_stride].xGrad);
    ++grid_line;
  }

  // Bottom border line.
  grid_line = grid.paddedData() + grid_stride * (height + 1) + 1;
  for (int x = 0; x < width; ++x) {
    grid_line[0].setBothGradients(grid_line[-grid_stride].xGrad);
    ++grid_line;
  }
  // Left and right border lines.
  grid_line = grid.paddedData() + grid_stride;
  for (int y = 0; y < height; ++y) {
    grid_line[0].setBothGradients(grid_line[1].xGrad);
    grid_line[grid_stride - 1].setBothGradients(grid_line[grid_stride - 2].xGrad);
    grid_line += grid_stride;
  }

  horizontalSobelInPlace(grid);
  verticalSobelInPlace(grid);
  // From horizontal and vertical gradients, calculate the directional one.
  grid_line = grid.data();
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      const Vec2f grad_vec(grid_line[x].xGrad, grid_line[x].yGrad);
      grid_line[x].dirDeriv = grad_vec.dot(direction);
      assert(std::fabs(grid_line[x].dirDeriv) <= 1.0);
    }

    grid_line += grid_stride;
  }
}  // TopBottomEdgeTracer::calcDirectionalDerivative

void TopBottomEdgeTracer::horizontalSobelInPlace(Grid<GridNode>& grid) {
  assert(grid.padding() == 1);

  const int width = grid.width();
  const int height = grid.height();
  const int grid_stride = grid.stride();

  // Do a vertical pass.
  for (int x = -1; x < width + 1; ++x) {
    GridNode* p_grid = grid.data() + x;
    float prev = p_grid[-grid_stride].xGrad;
    for (int y = 0; y < height; ++y) {
      const float cur = p_grid->xGrad;
      p_grid->xGrad = prev + cur + cur + p_grid[grid_stride].xGrad;
      prev = cur;
      p_grid += grid_stride;
    }
  }

  // Do a horizontal pass and write results.
  GridNode* grid_line = grid.data();
  for (int y = 0; y < height; ++y) {
    float prev = grid_line[-1].xGrad;
    for (int x = 0; x < width; ++x) {
      float cur = grid_line[x].xGrad;
      grid_line[x].xGrad = grid_line[x + 1].xGrad - prev;
      prev = cur;
    }
    grid_line += grid_stride;
  }
}

void TopBottomEdgeTracer::verticalSobelInPlace(Grid<GridNode>& grid) {
  assert(grid.padding() == 1);

  const int width = grid.width();
  const int height = grid.height();
  const int grid_stride = grid.stride();
  // Do a horizontal pass.
  GridNode* grid_line = grid.paddedData() + 1;
  for (int y = 0; y < height + 2; ++y) {
    float prev = grid_line[-1].yGrad;
    for (int x = 0; x < width; ++x) {
      float cur = grid_line[x].yGrad;
      grid_line[x].yGrad = prev + cur + cur + grid_line[x + 1].yGrad;
      prev = cur;
    }
    grid_line += grid_stride;
  }

  // Do a vertical pass and write resuts.
  for (int x = 0; x < width; ++x) {
    GridNode* p_grid = grid.data() + x;
    float prev = p_grid[-grid_stride].yGrad;
    for (int y = 0; y < height; ++y) {
      const float cur = p_grid->yGrad;
      p_grid->yGrad = p_grid[grid_stride].yGrad - prev;
      prev = cur;
      p_grid += grid_stride;
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

  int next_nbh_offsets[8];
  int prev_nbh_indexes[8];
  const int num_neighbours = initNeighbours(next_nbh_offsets, prev_nbh_indexes, grid.stride(), direction);

  while (!queue.empty()) {
    const int grid_idx = queue.front();
    GridNode* node = data + grid_idx;
    assert(node->pathCost >= 0);
    queue.pop();
    node->setHeapIdx(GridNode::INVALID_HEAP_IDX);

    for (int i = 0; i < num_neighbours; ++i) {
      const int nbh_grid_idx = grid_idx + next_nbh_offsets[i];
      GridNode* nbh_node = data + nbh_grid_idx;

      assert(std::fabs(node->dirDeriv) <= 1.0);
      const float new_cost
          = std::max<float>(node->pathCost, static_cast<const float&>(1.0f - std::fabs(node->dirDeriv)));
      if (new_cost < nbh_node->pathCost) {
        nbh_node->pathCost = new_cost;
        nbh_node->setPrevNeighbourIdx(prev_nbh_indexes[i]);
        if (nbh_node->heapIdx() == GridNode::INVALID_HEAP_IDX) {
          queue.push(nbh_grid_idx);
        } else {
          queue.reposition(nbh_node);
        }
      }
    }
  }
}  // TopBottomEdgeTracer::propagateShortestPaths

int TopBottomEdgeTracer::initNeighbours(int* next_nbh_offsets,
                                        int* prev_nbh_indexes,
                                        int stride,
                                        const Vec2f& direction) {
  const int candidate_offsets[] = {-stride - 1, -stride, -stride + 1, -1, 1, stride - 1, stride, stride + 1};

  const float candidate_vectors[8][2] = {{-1.0f, -1.0f}, {0.0f, -1.0f}, {1.0f, -1.0f}, {-1.0f, 0.0f},
                                         {1.0f, 0.0f},   {-1.0f, 1.0f}, {0.0f, 1.0f},  {1.0f, 1.0f}};

  static const int opposite_nbh_map[] = {7, 6, 5, 4, 3, 2, 1, 0};

  int out_idx = 0;
  for (int i = 0; i < 8; ++i) {
    const Vec2f vec(candidate_vectors[i][0], candidate_vectors[i][1]);
    if (vec.dot(direction) > 0) {
      next_nbh_offsets[out_idx] = candidate_offsets[i];
      prev_nbh_indexes[out_idx] = opposite_nbh_map[i];
      ++out_idx;
    }
  }

  return out_idx;
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

  const size_t num_best_paths = 2;  // Take N best paths.
  const int min_sqdist = 100 * 100;
  std::vector<Path> best_paths;

  GridLineTraverser traverser(line);
  while (traverser.hasNext()) {
    const QPoint pt(traverser.next());

    // intersectWithRect() ensures that.
    assert(pt.x() >= 0 && pt.y() >= 0 && pt.x() < width && pt.y() < height);

    const uint32_t offset = pt.y() * stride + pt.x();
    const GridNode* node = data + offset;

    // Find the closest path.
    Path* closest_path = nullptr;
    int closest_sqdist = std::numeric_limits<int>::max();
    for (Path& path : best_paths) {
      const QPoint delta(path.pt - pt);
      const int sqdist = delta.x() * delta.x() + delta.y() * delta.y();
      if (sqdist < closest_sqdist) {
        closest_path = &path;
        closest_sqdist = sqdist;
      }
    }

    if (closest_sqdist < min_sqdist) {
      // That's too close.
      if (node->pathCost < closest_path->cost) {
        closest_path->pt = pt;
        closest_path->cost = node->pathCost;
      }
      continue;
    }

    if (best_paths.size() < num_best_paths) {
      best_paths.emplace_back(pt, node->pathCost);
    } else {
      // Find the one to kick out (if any).
      for (Path& path : best_paths) {
        if (node->pathCost < path.cost) {
          path = Path(pt, node->pathCost);
          break;
        }
      }
    }
  }

  std::vector<QPoint> best_endpoints;

  for (const Path& path : best_paths) {
    if (path.cost < 0.95f) {
      best_endpoints.push_back(path.pt);
    }
  }

  return best_endpoints;
}  // TopBottomEdgeTracer::locateBestPathEndpoints

std::vector<QPoint> TopBottomEdgeTracer::tracePathFromEndpoint(const Grid<GridNode>& grid, const QPoint& endpoint) {
  static const int dx[8] = {-1, 0, 1, -1, 1, -1, 0, 1};
  static const int dy[8] = {-1, -1, -1, 0, 0, 1, 1, 1};

  const int stride = grid.stride();
  const int grid_offsets[8] = {-stride - 1, -stride, -stride + 1, -1, +1, +stride - 1, +stride, +stride + 1};

  const GridNode* const data = grid.data();
  std::vector<QPoint> path;

  QPoint pt(endpoint);
  int grid_offset = pt.x() + pt.y() * stride;
  for (;;) {
    path.push_back(pt);

    const GridNode* node = data + grid_offset;
    if (!node->hasPathContinuation()) {
      break;
    }

    const int nbh_idx = node->prevNeighbourIdx();
    grid_offset += grid_offsets[nbh_idx];
    pt += QPoint(dx[nbh_idx], dy[nbh_idx]);
  }

  return path;
}  // TopBottomEdgeTracer::tracePathFromEndpoint

std::vector<QPointF> TopBottomEdgeTracer::pathToSnake(const Grid<GridNode>& grid, const QPoint& endpoint) {
  const int max_dist = 15;  // Maximum distance between two snake knots.
  const int max_dist_sq = max_dist * max_dist;
  const int half_max_dist = max_dist / 2;
  const int half_max_dist_sq = half_max_dist * half_max_dist;

  static const int dx[8] = {-1, 0, 1, -1, 1, -1, 0, 1};
  static const int dy[8] = {-1, -1, -1, 0, 0, 1, 1, 1};

  const int stride = grid.stride();
  const int grid_offsets[8] = {-stride - 1, -stride, -stride + 1, -1, +1, +stride - 1, +stride, +stride + 1};

  const GridNode* const data = grid.data();
  std::vector<QPointF> snake;
  snake.emplace_back(endpoint);
  QPoint snake_tail(endpoint);

  QPoint pt(endpoint);
  int grid_offset = pt.x() + pt.y() * stride;
  for (;;) {
    const QPoint delta(pt - snake_tail);
    const int sqdist = delta.x() * delta.x() + delta.y() * delta.y();

    const GridNode* node = data + grid_offset;
    if (!node->hasPathContinuation()) {
      if (sqdist >= half_max_dist_sq) {
        snake.emplace_back(pt);
        snake_tail = pt;
      }
      break;
    }

    if (sqdist >= max_dist_sq) {
      snake.emplace_back(pt);
      snake_tail = pt;
    }

    const int nbh_idx = node->prevNeighbourIdx();
    grid_offset += grid_offsets[nbh_idx];
    pt += QPoint(dx[nbh_idx], dy[nbh_idx]);
  }

  return snake;
}  // TopBottomEdgeTracer::pathToSnake

void TopBottomEdgeTracer::gaussBlurGradient(Grid<GridNode>& grid) {
  using namespace boost::lambda;

  gaussBlurGeneric(QSize(grid.width(), grid.height()), 2.0f, 2.0f, grid.data(), grid.stride(),
                   bind(&GridNode::absDirDeriv, _1), grid.data(), grid.stride(), bind(&GridNode::blurred, _1) = _2);
}

Vec2f TopBottomEdgeTracer::downTheHillDirection(const QRectF& page_rect,
                                                const std::vector<QPointF>& snake,
                                                const Vec2f& bounds_dir) {
  assert(!snake.empty());

  // Take the centroid of a snake.
  QPointF centroid;
  for (const QPointF& pt : snake) {
    centroid += pt;
  }
  centroid /= snake.size();

  QLineF line(centroid, centroid + bounds_dir);
  lineBoundedByRect(line, page_rect);
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

  const size_t num_nodes = snake.size();
  if (num_nodes <= 1) {
    return;
  }

  float avg_dist = 0;
  for (size_t i = 1; i < num_nodes; ++i) {
    const Vec2f vec(snake[i] - snake[i - 1]);
    avg_dist += std::sqrt(vec.squaredNorm());
  }
  avg_dist /= num_nodes - 1;

  std::vector<Step> step_storage;

  Vec2f displacements[9];
  const int num_displacements = initDisplacementVectors(displacements, dir);

  const float elasticity_weight = 0.6f;
  const float bending_weight = 8.0f;
  const float external_weight = 0.4f;

  const float segment_dist_threshold = 1;

  for (int iteration = 0; iteration < 40; ++iteration) {
    step_storage.clear();

    std::vector<uint32_t> paths;
    std::vector<uint32_t> new_paths;

    for (size_t node_idx = 0; node_idx < num_nodes; ++node_idx) {
      const Vec2f pt(snake[node_idx]);
      const float cur_external_energy = interpolatedGridValue(grid, bind<float>(&GridNode::blurred, _1), pt, 1000);

      for (int displacement_idx = 0; displacement_idx < num_displacements; ++displacement_idx) {
        Step step;
        step.prevStepIdx = ~uint32_t(0);
        step.pt = pt + displacements[displacement_idx];
        step.pathCost = 0;

        const float adjusted_external_energy
            = interpolatedGridValue(grid, bind<float>(&GridNode::blurred, _1), step.pt, 1000);
        if (displacement_idx == 0) {
          step.pathCost += 100;
        } else if (cur_external_energy < 0.01) {
          if (cur_external_energy - adjusted_external_energy < 0.01f) {
            continue;
          }
        }

        step.pathCost += external_weight * adjusted_external_energy;

        float best_cost = NumericTraits<float>::max();
        uint32_t best_prev_step_idx = step.prevStepIdx;

        for (uint32_t prev_step_idx : paths) {
          const Step& prev_step = step_storage[prev_step_idx];
          float cost = prev_step.pathCost + step.pathCost;

          const Vec2f vec(step.pt - prev_step.pt);
          const auto vec_len = static_cast<const float>(std::sqrt(vec.squaredNorm()));
          if (vec_len < segment_dist_threshold) {
            cost += 1000;
          }

          // Elasticity.
          const auto dist_diff = std::fabs(avg_dist - vec_len);
          cost += elasticity_weight * (dist_diff / avg_dist);
          // Bending energy.
          if ((prev_step.prevStepIdx != ~uint32_t(0)) && (vec_len >= segment_dist_threshold)) {
            const Step& prev_prev_step = step_storage[prev_step.prevStepIdx];
            Vec2f prev_normal(prev_step.pt - prev_prev_step.pt);
            std::swap(prev_normal[0], prev_normal[1]);
            prev_normal[0] = -prev_normal[0];
            const auto prev_normal_len = static_cast<const float>(std::sqrt(prev_normal.squaredNorm()));
            if (prev_normal_len < segment_dist_threshold) {
              cost += 1000;
            } else {
              const float cos = vec.dot(prev_normal) / (vec_len * prev_normal_len);
              // cost += 0.7 * std::fabs(cos);
              cost += bending_weight * cos * cos;
            }
          }

          assert(cost < NumericTraits<float>::max());

          if (cost < best_cost) {
            best_cost = cost;
            best_prev_step_idx = prev_step_idx;
          }
        }

        step.prevStepIdx = best_prev_step_idx;
        if (best_prev_step_idx != ~uint32_t(0)) {
          step.pathCost = best_cost;
        }

        new_paths.push_back(static_cast<unsigned int&&>(step_storage.size()));
        step_storage.push_back(step);
      }
      assert(!new_paths.empty());
      paths.swap(new_paths);
      new_paths.clear();
    }

    uint32_t best_path_idx = ~uint32_t(0);
    float best_cost = NumericTraits<float>::max();
    for (uint32_t last_step_idx : paths) {
      const Step& step = step_storage[last_step_idx];
      if (step.pathCost < best_cost) {
        best_cost = step.pathCost;
        best_path_idx = last_step_idx;
      }
    }
    // Having found the best path, convert it back to a snake.
    snake.clear();
    uint32_t step_idx = best_path_idx;
    while (step_idx != ~uint32_t(0)) {
      const Step& step = step_storage[step_idx];
      snake.push_back(step.pt);
      step_idx = step.prevStepIdx;
    }
    assert(num_nodes == snake.size());
  }
}  // TopBottomEdgeTracer::downTheHillSnake

void TopBottomEdgeTracer::upTheHillSnake(std::vector<QPointF>& snake, const Grid<GridNode>& grid, const Vec2f dir) {
  using namespace boost::lambda;

  const size_t num_nodes = snake.size();
  if (num_nodes <= 1) {
    return;
  }

  float avg_dist = 0;
  for (size_t i = 1; i < num_nodes; ++i) {
    const Vec2f vec(snake[i] - snake[i - 1]);
    avg_dist += std::sqrt(vec.squaredNorm());
  }
  avg_dist /= num_nodes - 1;

  std::vector<Step> step_storage;

  Vec2f displacements[9];
  const int num_displacements = initDisplacementVectors(displacements, dir);
  for (int i = 0; i < num_displacements; ++i) {
    // We need more accuracy here.
    displacements[i] *= 0.5f;
  }

  const float elasticity_weight = 0.6f;
  const float bending_weight = 3.0f;
  const float external_weight = 2.0f;

  const float segment_dist_threshold = 1;

  for (int iteration = 0; iteration < 40; ++iteration) {
    step_storage.clear();

    std::vector<uint32_t> paths;
    std::vector<uint32_t> new_paths;

    for (size_t node_idx = 0; node_idx < num_nodes; ++node_idx) {
      const Vec2f pt(snake[node_idx]);
      const float cur_external_energy = -interpolatedGridValue(grid, bind<float>(&GridNode::absDirDeriv, _1), pt, 1000);

      for (int displacement_idx = 0; displacement_idx < num_displacements; ++displacement_idx) {
        Step step;
        step.prevStepIdx = ~uint32_t(0);
        step.pt = pt + displacements[displacement_idx];
        step.pathCost = 0;

        const float adjusted_external_energy
            = -interpolatedGridValue(grid, bind<float>(&GridNode::absDirDeriv, _1), step.pt, 1000);
        if ((displacement_idx == 0) && (adjusted_external_energy > -0.02)) {
          // Discorage staying on the spot if the gradient magnitude is too
          // small at that point.
          step.pathCost += 100;
        }

        step.pathCost += external_weight * adjusted_external_energy;

        float best_cost = NumericTraits<float>::max();
        uint32_t best_prev_step_idx = step.prevStepIdx;

        for (uint32_t prev_step_idx : paths) {
          const Step& prev_step = step_storage[prev_step_idx];
          float cost = prev_step.pathCost + step.pathCost;

          const Vec2f vec(step.pt - prev_step.pt);
          const auto vec_len = static_cast<const float>(std::sqrt(vec.squaredNorm()));
          if (vec_len < segment_dist_threshold) {
            cost += 1000;
          }

          // Elasticity.
          const auto dist_diff = std::fabs(avg_dist - vec_len);
          cost += elasticity_weight * (dist_diff / avg_dist);
          // Bending energy.
          if ((prev_step.prevStepIdx != ~uint32_t(0)) && (vec_len >= segment_dist_threshold)) {
            const Step& prev_prev_step = step_storage[prev_step.prevStepIdx];
            Vec2f prev_normal(prev_step.pt - prev_prev_step.pt);
            std::swap(prev_normal[0], prev_normal[1]);
            prev_normal[0] = -prev_normal[0];
            const auto prev_normal_len = static_cast<const float>(std::sqrt(prev_normal.squaredNorm()));
            if (prev_normal_len < segment_dist_threshold) {
              cost += 1000;
            } else {
              const float cos = vec.dot(prev_normal) / (vec_len * prev_normal_len);
              // cost += 0.7 * std::fabs(cos);
              cost += bending_weight * cos * cos;
            }
          }

          assert(cost < NumericTraits<float>::max());

          if (cost < best_cost) {
            best_cost = cost;
            best_prev_step_idx = prev_step_idx;
          }
        }

        step.prevStepIdx = best_prev_step_idx;
        if (best_prev_step_idx != ~uint32_t(0)) {
          step.pathCost = best_cost;
        }

        new_paths.push_back(static_cast<unsigned int&&>(step_storage.size()));
        step_storage.push_back(step);
      }
      assert(!new_paths.empty());
      paths.swap(new_paths);
      new_paths.clear();
    }

    uint32_t best_path_idx = ~uint32_t(0);
    float best_cost = NumericTraits<float>::max();
    for (uint32_t last_step_idx : paths) {
      const Step& step = step_storage[last_step_idx];
      if (step.pathCost < best_cost) {
        best_cost = step.pathCost;
        best_path_idx = last_step_idx;
      }
    }
    // Having found the best path, convert it back to a snake.
    snake.clear();
    uint32_t step_idx = best_path_idx;
    while (step_idx != ~uint32_t(0)) {
      const Step& step = step_storage[step_idx];
      snake.push_back(step.pt);
      step_idx = step.prevStepIdx;
    }
    assert(num_nodes == snake.size());
  }
}  // TopBottomEdgeTracer::upTheHillSnake

int TopBottomEdgeTracer::initDisplacementVectors(Vec2f vectors[], Vec2f valid_direction) {
  int out_idx = 0;
  // This one must always be present, and must be first, as we want to prefer it
  // over another one with exactly the same score.
  vectors[out_idx++] = Vec2f(0, 0);

  static const float dx[] = {-1, 0, 1, -1, 1, -1, 0, 1};

  static const float dy[] = {-1, -1, -1, 0, 0, 1, 1, 1};

  for (int i = 0; i < 8; ++i) {
    const Vec2f vec(dx[i], dy[i]);
    if (vec.dot(valid_direction) > 0) {
      vectors[out_idx++] = vec;
    }
  }

  return out_idx;
}

QImage TopBottomEdgeTracer::visualizeGradient(const Grid<GridNode>& grid, const QImage* background) {
  const int width = grid.width();
  const int height = grid.height();
  const int grid_stride = grid.stride();
  // First let's find the maximum and minimum values.
  float min_value = NumericTraits<float>::max();
  float max_value = NumericTraits<float>::min();

  const GridNode* grid_line = grid.data();
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      const float value = grid_line[x].dirDeriv;
      if (value < min_value) {
        min_value = value;
      } else if (value > max_value) {
        max_value = value;
      }
    }
    grid_line += grid_stride;
  }

  float scale = std::max(max_value, -min_value);
  if (scale > std::numeric_limits<float>::epsilon()) {
    scale = 255.0f / scale;
  }

  QImage overlay(width, height, QImage::Format_ARGB32_Premultiplied);
  auto* overlay_line = (uint32_t*) overlay.bits();
  const int overlay_stride = overlay.bytesPerLine() / 4;

  grid_line = grid.data();
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      const float value = grid_line[x].dirDeriv * scale;
      const int magnitude = qBound(0, static_cast<const int&>(std::round(std::fabs(value))), 255);
      if (value > 0) {
        // Red for positive gradients which indicate bottom edges.
        overlay_line[x] = qRgba(magnitude, 0, 0, magnitude);
      } else {
        overlay_line[x] = qRgba(0, 0, magnitude, magnitude);
      }
    }
    grid_line += grid_stride;
    overlay_line += overlay_stride;
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
  const int grid_stride = grid.stride();
  // First let's find the maximum and minimum values.
  float min_value = NumericTraits<float>::max();
  float max_value = NumericTraits<float>::min();

  const GridNode* grid_line = grid.data();
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      const float value = grid_line[x].blurred;
      if (value < min_value) {
        min_value = value;
      } else if (value > max_value) {
        max_value = value;
      }
    }
    grid_line += grid_stride;
  }

  float scale = std::max(max_value, -min_value);
  if (scale > std::numeric_limits<float>::epsilon()) {
    scale = 255.0f / scale;
  }

  QImage overlay(width, height, QImage::Format_ARGB32_Premultiplied);
  auto* overlay_line = (uint32_t*) overlay.bits();
  const int overlay_stride = overlay.bytesPerLine() / 4;

  grid_line = grid.data();
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      const float value = grid_line[x].blurred * scale;
      const int magnitude = qBound(0, static_cast<const int&>(std::round(std::fabs(value))), 255);
      overlay_line[x] = qRgba(magnitude, 0, 0, magnitude);
    }
    grid_line += grid_stride;
    overlay_line += overlay_stride;
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
                                           const std::vector<QPoint>& path_endpoints) {
  QImage canvas(background.convertToFormat(QImage::Format_RGB32));
  auto* const canvas_data = (uint32_t*) canvas.bits();
  const int canvas_stride = canvas.bytesPerLine() / 4;

  const int width = grid.width();
  const int height = grid.height();
  const int grid_stride = grid.stride();
  const GridNode* const grid_data = grid.data();

  const int nbh_canvas_offsets[8] = {-canvas_stride - 1, -canvas_stride, -canvas_stride + 1, -1, +1,
                                     +canvas_stride - 1, +canvas_stride, +canvas_stride + 1};
  const int nbh_grid_offsets[8]
      = {-grid_stride - 1, -grid_stride, -grid_stride + 1, -1, +1, +grid_stride - 1, +grid_stride, +grid_stride + 1};

  for (const QPoint path_endpoint : path_endpoints) {
    int grid_offset = path_endpoint.x() + path_endpoint.y() * grid_stride;
    int canvas_offset = path_endpoint.x() + path_endpoint.y() * canvas_stride;
    for (;;) {
      const GridNode* node = grid_data + grid_offset;
      canvas_data[canvas_offset] = 0x00ff0000;
      if (!node->hasPathContinuation()) {
        break;
      }

      const int nbh_idx = node->prevNeighbourIdx();
      grid_offset += nbh_grid_offsets[nbh_idx];
      canvas_offset += nbh_canvas_offsets[nbh_idx];
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
  auto* const canvas_data = (uint32_t*) canvas.bits();
  const int canvas_stride = canvas.bytesPerLine() / 4;

  for (const std::vector<QPoint>& path : paths) {
    for (QPoint pt : path) {
      canvas_data[pt.x() + pt.y() * canvas_stride] = 0x00ff0000;
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

  QPen snake_pen(QColor(0, 255, 0));
  snake_pen.setWidthF(1.5);

  QBrush knot_brush(QColor(255, 255, 0, 180));
  painter.setBrush(knot_brush);

  QRectF knot_rect(0, 0, 7, 7);

  for (const std::vector<QPointF>& snake : snakes) {
    if (snake.empty()) {
      continue;
    }

    painter.setPen(snake_pen);
    painter.drawPolyline(&snake[0], static_cast<int>(snake.size()));
    painter.setPen(Qt::NoPen);
    for (const QPointF& knot : snake) {
      knot_rect.moveCenter(knot);
      painter.drawEllipse(knot_rect);
    }
  }

  QPen bounds_pen(Qt::blue);
  bounds_pen.setWidthF(1.5);
  painter.setPen(bounds_pen);
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

  QPen polyline_pen(QColor(255, 0, 0, 100));
  polyline_pen.setWidthF(4.0);
  painter.setPen(polyline_pen);

  for (const std::vector<QPointF>& polyline : polylines) {
    if (polyline.empty()) {
      continue;
    }

    painter.drawPolyline(&polyline[0], static_cast<int>(polyline.size()));
  }

  QPen bounds_pen(Qt::blue);
  bounds_pen.setWidthF(1.5);
  painter.setPen(bounds_pen);
  painter.drawLine(bounds.first);
  painter.drawLine(bounds.second);

  return canvas;
}
}  // namespace dewarping