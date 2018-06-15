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

#include "TextLineRefiner.h"
#include <QDebug>
#include <QPainter>
#include <boost/foreach.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/lambda/lambda.hpp>
#include <cmath>
#include "DebugImages.h"
#include "NumericTraits.h"
#include "imageproc/GaussBlur.h"
#include "imageproc/Sobel.h"

using namespace imageproc;

namespace dewarping {
class TextLineRefiner::SnakeLength {
 public:
  explicit SnakeLength(const Snake& snake);

  float totalLength() const { return m_totalLength; }

  float avgSegmentLength() const { return m_avgSegmentLength; }

  float arcLengthAt(size_t node_idx) const { return m_integralLength[node_idx]; }

  float arcLengthFractionAt(size_t node_idx) const { return m_integralLength[node_idx] * m_rTotalLength; }

  float lengthFromTo(size_t from_node_idx, size_t to_node_idx) const {
    return m_integralLength[to_node_idx] - m_integralLength[from_node_idx];
  }

 private:
  std::vector<float> m_integralLength;
  float m_totalLength;
  float m_rTotalLength;  // Reciprocal of the above.
  float m_avgSegmentLength;
};


struct TextLineRefiner::FrenetFrame {
  Vec2f unitTangent;
  Vec2f unitDownNormal;
};


class TextLineRefiner::Optimizer {
 public:
  Optimizer(const Snake& snake, const Vec2f& unit_down_vec, float factor);

  bool thicknessAdjustment(Snake& snake, const Grid<float>& gradient);

  bool tangentMovement(Snake& snake, const Grid<float>& gradient);

  bool normalMovement(Snake& snake, const Grid<float>& gradient);

 private:
  static float calcExternalEnergy(const Grid<float>& gradient, const SnakeNode& node, Vec2f down_normal);

  static float calcElasticityEnergy(const SnakeNode& node1, const SnakeNode& node2, float avg_dist);

  static float calcBendingEnergy(const SnakeNode& node, const SnakeNode& prev_node, const SnakeNode& prev_prev_node);

  static const float m_elasticityWeight;
  static const float m_bendingWeight;
  static const float m_topExternalWeight;
  static const float m_bottomExternalWeight;
  const float m_factor;
  SnakeLength m_snakeLength;
  std::vector<FrenetFrame> m_frenetFrames;
};


TextLineRefiner::TextLineRefiner(const GrayImage& image, const Dpi& dpi, const Vec2f& unit_down_vector)
    : m_image(image), m_dpi(dpi), m_unitDownVec(unit_down_vector) {}

void TextLineRefiner::refine(std::list<std::vector<QPointF>>& polylines, const int iterations, DebugImages* dbg) const {
  if (polylines.empty()) {
    return;
  }

  std::vector<Snake> snakes;
  snakes.reserve(polylines.size());

  // Convert from polylines to snakes.
  for (const std::vector<QPointF>& polyline : polylines) {
    snakes.push_back(makeSnake(polyline, iterations));
  }

  if (dbg) {
    dbg->add(visualizeSnakes(snakes), "initial_snakes");
  }

  Grid<float> gradient(m_image.width(), m_image.height(), /*padding=*/0);

  // Start with a rather strong blur.
  float h_sigma = (4.0f / 200.f) * m_dpi.horizontal();
  float v_sigma = (4.0f / 200.f) * m_dpi.vertical();
  calcBlurredGradient(gradient, h_sigma, v_sigma);

  for (Snake& snake : snakes) {
    evolveSnake(snake, gradient, ON_CONVERGENCE_STOP);
  }
  if (dbg) {
    dbg->add(visualizeSnakes(snakes, &gradient), "evolved_snakes1");
  }

  // Less blurring this time.
  h_sigma *= 0.5f;
  v_sigma *= 0.5f;
  calcBlurredGradient(gradient, h_sigma, v_sigma);

  for (Snake& snake : snakes) {
    evolveSnake(snake, gradient, ON_CONVERGENCE_GO_FINER);
  }
  if (dbg) {
    dbg->add(visualizeSnakes(snakes, &gradient), "evolved_snakes2");
  }

  // Convert from snakes back to polylines.
  int i = -1;
  for (std::vector<QPointF>& polyline : polylines) {
    ++i;
    const Snake& snake = snakes[i];
    polyline.clear();
    for (const SnakeNode& node : snake.nodes) {
      polyline.push_back(node.center);
    }
  }
}  // TextLineRefiner::refine

void TextLineRefiner::calcBlurredGradient(Grid<float>& gradient, float h_sigma, float v_sigma) const {
  using namespace boost::lambda;

  const float downscale = 1.0f / (255.0f * 8.0f);
  Grid<float> vert_grad(m_image.width(), m_image.height(), /*padding=*/0);
  horizontalSobel<float>(m_image.width(), m_image.height(), m_image.data(), m_image.stride(), _1 * downscale,
                         gradient.data(), gradient.stride(), _1 = _2, _1, gradient.data(), gradient.stride(), _1 = _2);
  verticalSobel<float>(m_image.width(), m_image.height(), m_image.data(), m_image.stride(), _1 * downscale,
                       vert_grad.data(), vert_grad.stride(), _1 = _2, _1, gradient.data(), gradient.stride(),
                       _1 = _1 * m_unitDownVec[0] + _2 * m_unitDownVec[1]);
  Grid<float>().swap(vert_grad);  // Save memory.
  gaussBlurGeneric(m_image.size(), h_sigma, v_sigma, gradient.data(), gradient.stride(), _1, gradient.data(),
                   gradient.stride(), _1 = _2);
}

float TextLineRefiner::externalEnergyAt(const Grid<float>& gradient, const Vec2f& pos, float penalty_if_outside) {
  const auto x_base = static_cast<const float>(std::floor(pos[0]));
  const auto y_base = static_cast<const float>(std::floor(pos[1]));
  const auto x_base_i = (int) x_base;
  const auto y_base_i = (int) y_base;

  if ((x_base_i < 0) || (y_base_i < 0) || (x_base_i + 1 >= gradient.width()) || (y_base_i + 1 >= gradient.height())) {
    return penalty_if_outside;
  }

  const float x = pos[0] - x_base;
  const float y = pos[1] - y_base;
  const float x1 = 1.0f - x;
  const float y1 = 1.0f - y;

  const int stride = gradient.stride();
  const float* base = gradient.data() + y_base_i * stride + x_base_i;

  return base[0] * x1 * y1 + base[1] * x * y1 + base[stride] * x1 * y + base[stride + 1] * x * y;
}

TextLineRefiner::Snake TextLineRefiner::makeSnake(const std::vector<QPointF>& polyline, const int iterations) {
  float total_length = 0;

  const size_t polyline_size = polyline.size();
  for (size_t i = 1; i < polyline_size; ++i) {
    total_length += std::sqrt(Vec2f(polyline[i] - polyline[i - 1]).squaredNorm());
  }

  const auto points_in_snake = static_cast<const int>(total_length / 20);
  Snake snake;
  snake.iterationsRemaining = iterations;

  int points_inserted = 0;
  float base_t = 0;
  float next_insert_t = 0;
  for (size_t i = 1; i < polyline_size; ++i) {
    const Vec2f base(polyline[i - 1]);
    const Vec2f vec((polyline[i] - base));
    const auto next_t = static_cast<const float>(base_t + std::sqrt(vec.squaredNorm()));

    while (next_t >= next_insert_t) {
      const float fraction = (next_insert_t - base_t) / (next_t - base_t);
      SnakeNode node;
      node.center = base + fraction * vec;
      node.ribHalfLength = 4;
      snake.nodes.push_back(node);
      ++points_inserted;
      next_insert_t = total_length * points_inserted / (points_in_snake - 1);
    }

    base_t = next_t;
  }

  return snake;
}  // TextLineRefiner::makeSnake

void TextLineRefiner::calcFrenetFrames(std::vector<FrenetFrame>& frenet_frames,
                                       const Snake& snake,
                                       const SnakeLength& snake_length,
                                       const Vec2f& unit_down_vec) {
  const size_t num_nodes = snake.nodes.size();
  frenet_frames.resize(num_nodes);

  if (num_nodes == 0) {
    return;
  } else if (num_nodes == 1) {
    frenet_frames[0].unitTangent = Vec2f();
    frenet_frames[0].unitDownNormal = Vec2f();

    return;
  }

  // First segment.
  Vec2f first_segment(snake.nodes[1].center - snake.nodes[0].center);
  const float first_segment_len = snake_length.arcLengthAt(1);
  if (first_segment_len > std::numeric_limits<float>::epsilon()) {
    first_segment /= first_segment_len;
    frenet_frames.front().unitTangent = first_segment;
  }
  // Segments between first and last, exclusive.
  Vec2f prev_segment(first_segment);
  for (size_t i = 1; i < num_nodes - 1; ++i) {
    Vec2f next_segment(snake.nodes[i + 1].center - snake.nodes[i].center);
    const float next_segment_len = snake_length.lengthFromTo(i, i + 1);
    if (next_segment_len > std::numeric_limits<float>::epsilon()) {
      next_segment /= next_segment_len;
    }

    Vec2f tangent_vec(0.5 * (prev_segment + next_segment));
    const auto len = static_cast<const float>(std::sqrt(tangent_vec.squaredNorm()));
    if (len > std::numeric_limits<float>::epsilon()) {
      tangent_vec /= len;
    }
    frenet_frames[i].unitTangent = tangent_vec;

    prev_segment = next_segment;
  }

  // Last segments.
  Vec2f last_segment(snake.nodes[num_nodes - 1].center - snake.nodes[num_nodes - 2].center);
  const float last_segment_len = snake_length.lengthFromTo(num_nodes - 2, num_nodes - 1);
  if (last_segment_len > std::numeric_limits<float>::epsilon()) {
    last_segment /= last_segment_len;
    frenet_frames.back().unitTangent = last_segment;
  }

  // Calculate normals and make sure they point down.
  for (FrenetFrame& frame : frenet_frames) {
    frame.unitDownNormal = Vec2f(frame.unitTangent[1], -frame.unitTangent[0]);
    if (frame.unitDownNormal.dot(unit_down_vec) < 0) {
      frame.unitDownNormal = -frame.unitDownNormal;
    }
  }
}  // TextLineRefiner::calcFrenetFrames

void TextLineRefiner::evolveSnake(Snake& snake, const Grid<float>& gradient, const OnConvergence on_convergence) const {
  float factor = 1.0f;

  while (snake.iterationsRemaining > 0) {
    --snake.iterationsRemaining;

    Optimizer optimizer(snake, m_unitDownVec, factor);
    bool changed = false;
    changed |= optimizer.thicknessAdjustment(snake, gradient);
    changed |= optimizer.tangentMovement(snake, gradient);
    changed |= optimizer.normalMovement(snake, gradient);

    if (!changed) {
      // qDebug() << "Converged.  Iterations remaining = " << snake.iterationsRemaining;
      if (on_convergence == ON_CONVERGENCE_STOP) {
        break;
      } else {
        factor *= 0.5f;
      }
    }
  }
}

QImage TextLineRefiner::visualizeGradient(const Grid<float>& gradient) const {
  const int width = gradient.width();
  const int height = gradient.height();
  const int gradient_stride = gradient.stride();
  // First let's find the maximum and minimum values.
  float min_value = NumericTraits<float>::max();
  float max_value = NumericTraits<float>::min();

  const float* gradient_line = gradient.data();
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      const float value = gradient_line[x];
      if (value < min_value) {
        min_value = value;
      } else if (value > max_value) {
        max_value = value;
      }
    }
    gradient_line += gradient_stride;
  }

  float scale = std::max(max_value, -min_value);
  if (scale > std::numeric_limits<float>::epsilon()) {
    scale = 255.0f / scale;
  }

  QImage overlay(width, height, QImage::Format_ARGB32_Premultiplied);
  auto* overlay_line = (uint32_t*) overlay.bits();
  const int overlay_stride = overlay.bytesPerLine() / 4;

  gradient_line = gradient.data();
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      const float value = gradient_line[x] * scale;
      const int magnitude = qBound(0, static_cast<const int&>(std::round(std::fabs(value))), 255);
      if (value > 0) {
        // Red for positive gradients which indicate bottom edges.
        overlay_line[x] = qRgba(magnitude, 0, 0, magnitude);
      } else {
        overlay_line[x] = qRgba(0, 0, magnitude, magnitude);
      }
    }
    gradient_line += gradient_stride;
    overlay_line += overlay_stride;
  }

  QImage canvas(m_image.toQImage().convertToFormat(QImage::Format_ARGB32_Premultiplied));
  QPainter painter(&canvas);
  painter.drawImage(0, 0, overlay);

  return canvas;
}  // TextLineRefiner::visualizeGradient

QImage TextLineRefiner::visualizeSnakes(const std::vector<Snake>& snakes, const Grid<float>* gradient) const {
  QImage canvas;
  if (gradient) {
    canvas = visualizeGradient(*gradient);
  } else {
    canvas = m_image.toQImage().convertToFormat(QImage::Format_ARGB32_Premultiplied);
  }

  QPainter painter(&canvas);
  painter.setRenderHint(QPainter::Antialiasing);

  QPen top_pen(QColor(0, 0, 255));
  top_pen.setWidthF(1.5);

  QPen bottom_pen(QColor(255, 0, 0));
  bottom_pen.setWidthF(1.5);

  QPen middle_pen(QColor(255, 0, 255));
  middle_pen.setWidth(static_cast<int>(1.5));

  QBrush knot_brush(QColor(255, 255, 0, 180));
  painter.setBrush(knot_brush);

  QRectF knot_rect(0, 0, 7, 7);
  std::vector<FrenetFrame> frenet_frames;

  for (const Snake& snake : snakes) {
    const SnakeLength snake_length(snake);
    calcFrenetFrames(frenet_frames, snake, snake_length, m_unitDownVec);
    QVector<QPointF> top_polyline;
    QVector<QPointF> middle_polyline;
    QVector<QPointF> bottom_polyline;

    const size_t num_nodes = snake.nodes.size();
    for (size_t i = 0; i < num_nodes; ++i) {
      const QPointF mid(snake.nodes[i].center + QPointF(0.5, 0.5));
      const QPointF top(mid - snake.nodes[i].ribHalfLength * frenet_frames[i].unitDownNormal);
      const QPointF bottom(mid + snake.nodes[i].ribHalfLength * frenet_frames[i].unitDownNormal);
      top_polyline << top;
      middle_polyline << mid;
      bottom_polyline << bottom;
    }

    // Draw polylines.
    painter.setPen(top_pen);
    painter.drawPolyline(top_polyline);

    painter.setPen(bottom_pen);
    painter.drawPolyline(bottom_polyline);

    painter.setPen(middle_pen);
    painter.drawPolyline(middle_polyline);

    // Draw knots.
    painter.setPen(Qt::NoPen);
    for (const QPointF& pt : middle_polyline) {
      knot_rect.moveCenter(pt);
      painter.drawEllipse(knot_rect);
    }
  }

  return canvas;
}  // TextLineRefiner::visualizeSnakes

/*============================ SnakeLength =============================*/

TextLineRefiner::SnakeLength::SnakeLength(const Snake& snake)
    : m_integralLength(snake.nodes.size()), m_totalLength(), m_rTotalLength(), m_avgSegmentLength() {
  const size_t num_nodes = snake.nodes.size();
  float arc_length_accum = 0;
  for (size_t i = 1; i < num_nodes; ++i) {
    const Vec2f vec(snake.nodes[i].center - snake.nodes[i - 1].center);
    arc_length_accum += std::sqrt(vec.squaredNorm());
    m_integralLength[i] = arc_length_accum;
  }
  m_totalLength = arc_length_accum;
  if (m_totalLength > std::numeric_limits<float>::epsilon()) {
    m_rTotalLength = 1.0f / m_totalLength;
  }
  if (num_nodes > 1) {
    m_avgSegmentLength = m_totalLength / (num_nodes - 1);
  }
}

/*=========================== Optimizer =============================*/

const float TextLineRefiner::Optimizer::m_elasticityWeight = 0.2f;
const float TextLineRefiner::Optimizer::m_bendingWeight = 1.8f;
const float TextLineRefiner::Optimizer::m_topExternalWeight = 1.0f;
const float TextLineRefiner::Optimizer::m_bottomExternalWeight = 1.0f;

TextLineRefiner::Optimizer::Optimizer(const Snake& snake, const Vec2f& unit_down_vec, float factor)
    : m_factor(factor), m_snakeLength(snake) {
  calcFrenetFrames(m_frenetFrames, snake, m_snakeLength, unit_down_vec);
}

bool TextLineRefiner::Optimizer::thicknessAdjustment(Snake& snake, const Grid<float>& gradient) {
  const size_t num_nodes = snake.nodes.size();

  const float rib_adjustments[] = {0.0f * m_factor, 0.5f * m_factor, -0.5f * m_factor};
  enum { NUM_RIB_ADJUSTMENTS = sizeof(rib_adjustments) / sizeof(rib_adjustments[0]) };

  int best_i = 0;
  int best_j = 0;
  float best_cost = NumericTraits<float>::max();
  for (int i = 0; i < NUM_RIB_ADJUSTMENTS; ++i) {
    const float head_rib = snake.nodes.front().ribHalfLength + rib_adjustments[i];
    if (head_rib <= std::numeric_limits<float>::epsilon()) {
      continue;
    }

    for (int j = 0; j < NUM_RIB_ADJUSTMENTS; ++j) {
      const float tail_rib = snake.nodes.back().ribHalfLength + rib_adjustments[j];
      if (tail_rib <= std::numeric_limits<float>::epsilon()) {
        continue;
      }

      float cost = 0;
      for (size_t node_idx = 0; node_idx < num_nodes; ++node_idx) {
        const float t = m_snakeLength.arcLengthFractionAt(node_idx);
        const float rib = head_rib + t * (tail_rib - head_rib);
        const Vec2f down_normal(m_frenetFrames[node_idx].unitDownNormal);

        SnakeNode node(snake.nodes[node_idx]);
        node.ribHalfLength = rib;
        cost += calcExternalEnergy(gradient, node, down_normal);
      }
      if (cost < best_cost) {
        best_cost = cost;
        best_i = i;
        best_j = j;
      }
    }
  }
  const float head_rib = snake.nodes.front().ribHalfLength + rib_adjustments[best_i];
  const float tail_rib = snake.nodes.back().ribHalfLength + rib_adjustments[best_j];
  for (size_t node_idx = 0; node_idx < num_nodes; ++node_idx) {
    const float t = m_snakeLength.arcLengthFractionAt(node_idx);
    snake.nodes[node_idx].ribHalfLength = head_rib + t * (tail_rib - head_rib);
    // Note that we need to recalculate inner ribs even if outer ribs
    // didn't change, as movement of ribs in tangent direction affects
    // interpolation.
  }

  return rib_adjustments[best_i] != 0 || rib_adjustments[best_j] != 0;
}  // TextLineRefiner::Optimizer::thicknessAdjustment

bool TextLineRefiner::Optimizer::tangentMovement(Snake& snake, const Grid<float>& gradient) {
  const size_t num_nodes = snake.nodes.size();
  if (num_nodes < 3) {
    return false;
  }

  const float tangent_movements[] = {0.0f * m_factor, 1.0f * m_factor, -1.0f * m_factor};
  enum { NUM_TANGENT_MOVEMENTS = sizeof(tangent_movements) / sizeof(tangent_movements[0]) };

  std::vector<uint32_t> paths;
  std::vector<uint32_t> new_paths;
  std::vector<Step> step_storage;
  // Note that we don't move the first and the last node in tangent direction.
  paths.push_back(static_cast<unsigned int&&>(step_storage.size()));
  step_storage.emplace_back();
  step_storage.back().prevStepIdx = ~uint32_t(0);
  step_storage.back().node = snake.nodes.front();
  step_storage.back().pathCost = 0;

  for (size_t node_idx = 1; node_idx < num_nodes - 1; ++node_idx) {
    const Vec2f initial_pos(snake.nodes[node_idx].center);
    const float rib = snake.nodes[node_idx].ribHalfLength;
    const Vec2f unit_tangent(m_frenetFrames[node_idx].unitTangent);
    const Vec2f down_normal(m_frenetFrames[node_idx].unitDownNormal);

    for (float tangent_movement : tangent_movements) {
      Step step;
      step.prevStepIdx = ~uint32_t(0);
      step.node.center = initial_pos + tangent_movement * unit_tangent;
      step.node.ribHalfLength = rib;
      step.pathCost = NumericTraits<float>::max();

      float base_cost = calcExternalEnergy(gradient, step.node, down_normal);

      if (node_idx == num_nodes - 2) {
        // Take into account the distance to the last node as well.
        base_cost += calcElasticityEnergy(step.node, snake.nodes.back(), m_snakeLength.avgSegmentLength());
      }

      // Now find the best step for the previous node to combine with.
      for (uint32_t prev_step_idx : paths) {
        const Step& prev_step = step_storage[prev_step_idx];
        const float cost = base_cost + prev_step.pathCost
                           + calcElasticityEnergy(step.node, prev_step.node, m_snakeLength.avgSegmentLength());

        if (cost < step.pathCost) {
          step.pathCost = cost;
          step.prevStepIdx = prev_step_idx;
        }
      }
      assert(step.prevStepIdx != ~uint32_t(0));
      new_paths.push_back(static_cast<unsigned int&&>(step_storage.size()));
      step_storage.push_back(step);
    }
    assert(!new_paths.empty());
    paths.swap(new_paths);
    new_paths.clear();
  }

  // Find the best overall path.
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
  float max_sqdist = 0;
  uint32_t step_idx = best_path_idx;
  for (auto node_idx = static_cast<int>(num_nodes - 2); node_idx > 0; --node_idx) {
    assert(step_idx != ~uint32_t(0));
    const Step& step = step_storage[step_idx];
    SnakeNode& node = snake.nodes[node_idx];

    const float sqdist = (node.center - step.node.center).squaredNorm();
    max_sqdist = std::max<float>(max_sqdist, sqdist);

    node = step.node;
    step_idx = step.prevStepIdx;
  }

  return max_sqdist > std::numeric_limits<float>::epsilon();
}  // TextLineRefiner::Optimizer::tangentMovement

bool TextLineRefiner::Optimizer::normalMovement(Snake& snake, const Grid<float>& gradient) {
  const size_t num_nodes = snake.nodes.size();
  if (num_nodes < 3) {
    return false;
  }

  const float normal_movements[] = {0.0f * m_factor, 1.0f * m_factor, -1.0f * m_factor};
  enum { NUM_NORMAL_MOVEMENTS = sizeof(normal_movements) / sizeof(normal_movements[0]) };

  std::vector<uint32_t> paths;
  std::vector<uint32_t> new_paths;
  std::vector<Step> step_storage;
  // The first two nodes pose a problem for us.  These nodes don't have two predecessors,
  // and therefore we can't take bending into the account.  We could take the followers
  // instead of the ancestors, but then this follower is going to move itself, making
  // our calculations less accurate.  The proper solution is to provide not N but N*N
  // paths to the 3rd node, each path corresponding to a combination of movement of
  // the first and the second node.  That's the approach we are taking here.
  for (float normal_movement : normal_movements) {
    const auto prev_step_idx = static_cast<const uint32_t>(step_storage.size());
    {
      // Movements of the first node.
      const Vec2f down_normal(m_frenetFrames[0].unitDownNormal);
      Step step;
      step.node.center = snake.nodes[0].center + normal_movement * down_normal;
      step.node.ribHalfLength = snake.nodes[0].ribHalfLength;
      step.prevStepIdx = ~uint32_t(0);
      step.pathCost = calcExternalEnergy(gradient, step.node, down_normal);

      step_storage.push_back(step);
    }

    for (float j : normal_movements) {
      // Movements of the second node.
      const Vec2f down_normal(m_frenetFrames[1].unitDownNormal);

      Step step;
      step.node.center = snake.nodes[1].center + j * down_normal;
      step.node.ribHalfLength = snake.nodes[1].ribHalfLength;
      step.prevStepIdx = prev_step_idx;
      step.pathCost = step_storage[prev_step_idx].pathCost + calcExternalEnergy(gradient, step.node, down_normal);

      paths.push_back(static_cast<unsigned int&&>(step_storage.size()));
      step_storage.push_back(step);
    }
  }

  for (size_t node_idx = 2; node_idx < num_nodes; ++node_idx) {
    const SnakeNode& node = snake.nodes[node_idx];
    const Vec2f down_normal(m_frenetFrames[node_idx].unitDownNormal);

    for (float normal_movement : normal_movements) {
      Step step;
      step.prevStepIdx = ~uint32_t(0);
      step.node.center = node.center + normal_movement * down_normal;
      step.node.ribHalfLength = node.ribHalfLength;
      step.pathCost = NumericTraits<float>::max();

      const float base_cost = calcExternalEnergy(gradient, step.node, down_normal);

      // Now find the best step for the previous node to combine with.
      for (uint32_t prev_step_idx : paths) {
        const Step& prev_step = step_storage[prev_step_idx];
        const Step& prev_prev_step = step_storage[prev_step.prevStepIdx];

        const float cost
            = base_cost + prev_step.pathCost + calcBendingEnergy(step.node, prev_step.node, prev_prev_step.node);

        if (cost < step.pathCost) {
          step.pathCost = cost;
          step.prevStepIdx = prev_step_idx;
        }
      }
      assert(step.prevStepIdx != ~uint32_t(0));
      new_paths.push_back(static_cast<unsigned int&&>(step_storage.size()));
      step_storage.push_back(step);
    }
    assert(!new_paths.empty());
    paths.swap(new_paths);
    new_paths.clear();
  }

  // Find the best overall path.
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
  float max_sqdist = 0;
  uint32_t step_idx = best_path_idx;
  for (auto node_idx = static_cast<int>(num_nodes - 1); node_idx >= 0; --node_idx) {
    assert(step_idx != ~uint32_t(0));
    const Step& step = step_storage[step_idx];
    SnakeNode& node = snake.nodes[node_idx];

    const float sqdist = (node.center - step.node.center).squaredNorm();
    max_sqdist = std::max<float>(max_sqdist, sqdist);

    node = step.node;
    step_idx = step.prevStepIdx;
  }

  return max_sqdist > std::numeric_limits<float>::epsilon();
}  // TextLineRefiner::Optimizer::normalMovement

float TextLineRefiner::Optimizer::calcExternalEnergy(const Grid<float>& gradient,
                                                     const SnakeNode& node,
                                                     const Vec2f down_normal) {
  const Vec2f top(node.center - node.ribHalfLength * down_normal);
  const Vec2f bottom(node.center + node.ribHalfLength * down_normal);

  const float top_grad = externalEnergyAt(gradient, top, 0.0f);
  const float bottom_grad = externalEnergyAt(gradient, bottom, 0.0f);

  // Surprisingly, it turns out it's a bad idea to penalize for the opposite
  // sign in the gradient.  Sometimes a snake's edge has to move over the
  // "wrong" gradient ridge before it gets into a good position.
  // Those std::min and std::max prevent such penalties.
  const float top_energy = m_topExternalWeight * std::min<float>(top_grad, 0.0f);
  const float bottom_energy = m_bottomExternalWeight * std::max<float>(bottom_grad, 0.0f);

  // Positive gradient indicates the bottom edge and vice versa.
  // Note that negative energies are fine with us - the less the better.
  return top_energy - bottom_energy;
}

float TextLineRefiner::Optimizer::calcElasticityEnergy(const SnakeNode& node1, const SnakeNode& node2, float avg_dist) {
  const Vec2f vec(node1.center - node2.center);
  const auto vec_len = static_cast<const float>(std::sqrt(vec.squaredNorm()));

  if (vec_len < 1.0f) {
    return 1000.0f;  // Penalty for moving too close to another node.
  }

  const auto dist_diff = std::fabs(avg_dist - vec_len);

  return m_elasticityWeight * (dist_diff / avg_dist);
}

float TextLineRefiner::Optimizer::calcBendingEnergy(const SnakeNode& node,
                                                    const SnakeNode& prev_node,
                                                    const SnakeNode& prev_prev_node) {
  const Vec2f vec(node.center - prev_node.center);
  const auto vec_len = static_cast<const float>(std::sqrt(vec.squaredNorm()));

  if (vec_len < 1.0f) {
    return 1000.0f;  // Penalty for moving too close to another node.
  }

  const Vec2f prev_vec(prev_node.center - prev_prev_node.center);
  const auto prev_vec_len = static_cast<const float>(std::sqrt(prev_vec.squaredNorm()));
  if (prev_vec_len < 1.0f) {
    return 1000.0f;  // Penalty for moving too close to another node.
  }

  const Vec2f bend_vec(vec / vec_len - prev_vec / prev_vec_len);

  return m_bendingWeight * bend_vec.squaredNorm();
}
}  // namespace dewarping