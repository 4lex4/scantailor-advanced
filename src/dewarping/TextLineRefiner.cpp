// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "TextLineRefiner.h"

#include <GaussBlur.h>
#include <Sobel.h>

#include <QDebug>
#include <QPainter>
#include <boost/foreach.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/lambda/lambda.hpp>
#include <cmath>

#include "DebugImages.h"
#include "NumericTraits.h"

using namespace imageproc;

namespace dewarping {
class TextLineRefiner::SnakeLength {
 public:
  explicit SnakeLength(const Snake& snake);

  float totalLength() const { return m_totalLength; }

  float avgSegmentLength() const { return m_avgSegmentLength; }

  float arcLengthAt(size_t nodeIdx) const { return m_integralLength[nodeIdx]; }

  float arcLengthFractionAt(size_t nodeIdx) const { return m_integralLength[nodeIdx] * m_reciprocalTotalLength; }

  float lengthFromTo(size_t fromNodeIdx, size_t toNodeIdx) const {
    return m_integralLength[toNodeIdx] - m_integralLength[fromNodeIdx];
  }

 private:
  std::vector<float> m_integralLength;
  float m_totalLength;
  float m_reciprocalTotalLength;
  float m_avgSegmentLength;
};


struct TextLineRefiner::FrenetFrame {
  Vec2f unitTangent;
  Vec2f unitDownNormal;
};


class TextLineRefiner::Optimizer {
 public:
  Optimizer(const Snake& snake, const Vec2f& unitDownVec, float factor);

  bool thicknessAdjustment(Snake& snake, const Grid<float>& gradient);

  bool tangentMovement(Snake& snake, const Grid<float>& gradient);

  bool normalMovement(Snake& snake, const Grid<float>& gradient);

 private:
  static float calcExternalEnergy(const Grid<float>& gradient, const SnakeNode& node, Vec2f downNormal);

  static float calcElasticityEnergy(const SnakeNode& node1, const SnakeNode& node2, float avgDist);

  static float calcBendingEnergy(const SnakeNode& node, const SnakeNode& prevNode, const SnakeNode& prevPrevNode);

  static const float m_elasticityWeight;
  static const float m_bendingWeight;
  static const float m_topExternalWeight;
  static const float m_bottomExternalWeight;
  const float m_factor;
  SnakeLength m_snakeLength;
  std::vector<FrenetFrame> m_frenetFrames;
};


TextLineRefiner::TextLineRefiner(const GrayImage& image, const Dpi& dpi, const Vec2f& unitDownVector)
    : m_image(image), m_dpi(dpi), m_unitDownVec(unitDownVector) {}

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
  float hSigma = (4.0f / 200.f) * m_dpi.horizontal();
  float vSigma = (4.0f / 200.f) * m_dpi.vertical();
  calcBlurredGradient(gradient, hSigma, vSigma);

  for (Snake& snake : snakes) {
    evolveSnake(snake, gradient, ON_CONVERGENCE_STOP);
  }
  if (dbg) {
    dbg->add(visualizeSnakes(snakes, &gradient), "evolved_snakes1");
  }

  // Less blurring this time.
  hSigma *= 0.5f;
  vSigma *= 0.5f;
  calcBlurredGradient(gradient, hSigma, vSigma);

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

void TextLineRefiner::calcBlurredGradient(Grid<float>& gradient, float hSigma, float vSigma) const {
  using namespace boost::lambda;

  const float downscale = 1.0f / (255.0f * 8.0f);
  Grid<float> vertGrad(m_image.width(), m_image.height(), /*padding=*/0);
  horizontalSobel<float>(m_image.width(), m_image.height(), m_image.data(), m_image.stride(), _1 * downscale,
                         gradient.data(), gradient.stride(), _1 = _2, _1, gradient.data(), gradient.stride(), _1 = _2);
  verticalSobel<float>(m_image.width(), m_image.height(), m_image.data(), m_image.stride(), _1 * downscale,
                       vertGrad.data(), vertGrad.stride(), _1 = _2, _1, gradient.data(), gradient.stride(),
                       _1 = _1 * m_unitDownVec[0] + _2 * m_unitDownVec[1]);
  Grid<float>().swap(vertGrad);  // Save memory.
  gaussBlurGeneric(m_image.size(), hSigma, vSigma, gradient.data(), gradient.stride(), _1, gradient.data(),
                   gradient.stride(), _1 = _2);
}

float TextLineRefiner::externalEnergyAt(const Grid<float>& gradient, const Vec2f& pos, float penaltyIfOutside) {
  const auto xBase = static_cast<float>(std::floor(pos[0]));
  const auto yBase = static_cast<float>(std::floor(pos[1]));
  const auto xBaseI = (int) xBase;
  const auto yBaseI = (int) yBase;

  if ((xBaseI < 0) || (yBaseI < 0) || (xBaseI + 1 >= gradient.width()) || (yBaseI + 1 >= gradient.height())) {
    return penaltyIfOutside;
  }

  const float x = pos[0] - xBase;
  const float y = pos[1] - yBase;
  const float x1 = 1.0f - x;
  const float y1 = 1.0f - y;

  const int stride = gradient.stride();
  const float* base = gradient.data() + yBaseI * stride + xBaseI;
  return base[0] * x1 * y1 + base[1] * x * y1 + base[stride] * x1 * y + base[stride + 1] * x * y;
}

TextLineRefiner::Snake TextLineRefiner::makeSnake(const std::vector<QPointF>& polyline, const int iterations) {
  float totalLength = 0;

  const size_t polylineSize = polyline.size();
  for (size_t i = 1; i < polylineSize; ++i) {
    totalLength += std::sqrt(Vec2f(polyline[i] - polyline[i - 1]).squaredNorm());
  }

  const auto pointsInSnake = static_cast<int>(totalLength / 20);
  Snake snake;
  snake.iterationsRemaining = iterations;

  int pointsInserted = 0;
  float baseT = 0;
  float nextInsertT = 0;
  for (size_t i = 1; i < polylineSize; ++i) {
    const Vec2f base(polyline[i - 1]);
    const Vec2f vec((polyline[i] - base));
    const auto nextT = static_cast<float>(baseT + std::sqrt(vec.squaredNorm()));

    while (nextT >= nextInsertT) {
      const float fraction = (nextInsertT - baseT) / (nextT - baseT);
      SnakeNode node;
      node.center = base + fraction * vec;
      node.ribHalfLength = 4;
      snake.nodes.push_back(node);
      ++pointsInserted;
      nextInsertT = totalLength * pointsInserted / (pointsInSnake - 1);
    }

    baseT = nextT;
  }
  return snake;
}  // TextLineRefiner::makeSnake

void TextLineRefiner::calcFrenetFrames(std::vector<FrenetFrame>& frenetFrames,
                                       const Snake& snake,
                                       const SnakeLength& snakeLength,
                                       const Vec2f& unitDownVec) {
  const size_t numNodes = snake.nodes.size();
  frenetFrames.resize(numNodes);

  if (numNodes == 0) {
    return;
  } else if (numNodes == 1) {
    frenetFrames[0].unitTangent = Vec2f();
    frenetFrames[0].unitDownNormal = Vec2f();
    return;
  }

  // First segment.
  Vec2f firstSegment(snake.nodes[1].center - snake.nodes[0].center);
  const float firstSegmentLen = snakeLength.arcLengthAt(1);
  if (firstSegmentLen > std::numeric_limits<float>::epsilon()) {
    firstSegment /= firstSegmentLen;
    frenetFrames.front().unitTangent = firstSegment;
  }
  // Segments between first and last, exclusive.
  Vec2f prevSegment(firstSegment);
  for (size_t i = 1; i < numNodes - 1; ++i) {
    Vec2f nextSegment(snake.nodes[i + 1].center - snake.nodes[i].center);
    const float nextSegmentLen = snakeLength.lengthFromTo(i, i + 1);
    if (nextSegmentLen > std::numeric_limits<float>::epsilon()) {
      nextSegment /= nextSegmentLen;
    }

    Vec2f tangentVec(0.5f * (prevSegment + nextSegment));
    const auto len = static_cast<float>(std::sqrt(tangentVec.squaredNorm()));
    if (len > std::numeric_limits<float>::epsilon()) {
      tangentVec /= len;
    }
    frenetFrames[i].unitTangent = tangentVec;

    prevSegment = nextSegment;
  }

  // Last segments.
  Vec2f lastSegment(snake.nodes[numNodes - 1].center - snake.nodes[numNodes - 2].center);
  const float lastSegmentLen = snakeLength.lengthFromTo(numNodes - 2, numNodes - 1);
  if (lastSegmentLen > std::numeric_limits<float>::epsilon()) {
    lastSegment /= lastSegmentLen;
    frenetFrames.back().unitTangent = lastSegment;
  }

  // Calculate normals and make sure they point down.
  for (FrenetFrame& frame : frenetFrames) {
    frame.unitDownNormal = Vec2f(frame.unitTangent[1], -frame.unitTangent[0]);
    if (frame.unitDownNormal.dot(unitDownVec) < 0) {
      frame.unitDownNormal = -frame.unitDownNormal;
    }
  }
}  // TextLineRefiner::calcFrenetFrames

void TextLineRefiner::evolveSnake(Snake& snake, const Grid<float>& gradient, const OnConvergence onConvergence) const {
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
      if (onConvergence == ON_CONVERGENCE_STOP) {
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
  const int gradientStride = gradient.stride();
  // First let's find the maximum and minimum values.
  float minValue = NumericTraits<float>::max();
  float maxValue = NumericTraits<float>::min();

  const float* gradientLine = gradient.data();
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      const float value = gradientLine[x];
      if (value < minValue) {
        minValue = value;
      } else if (value > maxValue) {
        maxValue = value;
      }
    }
    gradientLine += gradientStride;
  }

  float scale = std::max(maxValue, -minValue);
  if (scale > std::numeric_limits<float>::epsilon()) {
    scale = 255.0f / scale;
  }

  QImage overlay(width, height, QImage::Format_ARGB32_Premultiplied);
  auto* overlayLine = (uint32_t*) overlay.bits();
  const int overlayStride = overlay.bytesPerLine() / 4;

  gradientLine = gradient.data();
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      const float value = gradientLine[x] * scale;
      const int magnitude = qBound(0, static_cast<const int&>(std::round(std::fabs(value))), 255);
      if (value > 0) {
        // Red for positive gradients which indicate bottom edges.
        overlayLine[x] = qRgba(magnitude, 0, 0, magnitude);
      } else {
        overlayLine[x] = qRgba(0, 0, magnitude, magnitude);
      }
    }
    gradientLine += gradientStride;
    overlayLine += overlayStride;
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

  QPen topPen(QColor(0, 0, 255));
  topPen.setWidthF(1.5);

  QPen bottomPen(QColor(255, 0, 0));
  bottomPen.setWidthF(1.5);

  QPen middlePen(QColor(255, 0, 255));
  middlePen.setWidth(static_cast<int>(1.5));

  QBrush knotBrush(QColor(255, 255, 0, 180));
  painter.setBrush(knotBrush);

  QRectF knotRect(0, 0, 7, 7);
  std::vector<FrenetFrame> frenetFrames;

  for (const Snake& snake : snakes) {
    const SnakeLength snakeLength(snake);
    calcFrenetFrames(frenetFrames, snake, snakeLength, m_unitDownVec);
    QVector<QPointF> topPolyline;
    QVector<QPointF> middlePolyline;
    QVector<QPointF> bottomPolyline;

    const size_t numNodes = snake.nodes.size();
    for (size_t i = 0; i < numNodes; ++i) {
      const QPointF mid(snake.nodes[i].center + QPointF(0.5, 0.5));
      const QPointF top(mid - snake.nodes[i].ribHalfLength * frenetFrames[i].unitDownNormal);
      const QPointF bottom(mid + snake.nodes[i].ribHalfLength * frenetFrames[i].unitDownNormal);
      topPolyline << top;
      middlePolyline << mid;
      bottomPolyline << bottom;
    }

    // Draw polylines.
    painter.setPen(topPen);
    painter.drawPolyline(topPolyline);

    painter.setPen(bottomPen);
    painter.drawPolyline(bottomPolyline);

    painter.setPen(middlePen);
    painter.drawPolyline(middlePolyline);

    // Draw knots.
    painter.setPen(Qt::NoPen);
    for (const QPointF& pt : middlePolyline) {
      knotRect.moveCenter(pt);
      painter.drawEllipse(knotRect);
    }
  }
  return canvas;
}  // TextLineRefiner::visualizeSnakes

/*============================ SnakeLength =============================*/

TextLineRefiner::SnakeLength::SnakeLength(const Snake& snake)
    : m_integralLength(snake.nodes.size()), m_totalLength(), m_reciprocalTotalLength(), m_avgSegmentLength() {
  const size_t numNodes = snake.nodes.size();
  float arcLengthAccum = 0;
  for (size_t i = 1; i < numNodes; ++i) {
    const Vec2f vec(snake.nodes[i].center - snake.nodes[i - 1].center);
    arcLengthAccum += std::sqrt(vec.squaredNorm());
    m_integralLength[i] = arcLengthAccum;
  }
  m_totalLength = arcLengthAccum;
  if (m_totalLength > std::numeric_limits<float>::epsilon()) {
    m_reciprocalTotalLength = 1.0f / m_totalLength;
  }
  if (numNodes > 1) {
    m_avgSegmentLength = m_totalLength / (numNodes - 1);
  }
}

/*=========================== Optimizer =============================*/

const float TextLineRefiner::Optimizer::m_elasticityWeight = 0.2f;
const float TextLineRefiner::Optimizer::m_bendingWeight = 1.8f;
const float TextLineRefiner::Optimizer::m_topExternalWeight = 1.0f;
const float TextLineRefiner::Optimizer::m_bottomExternalWeight = 1.0f;

TextLineRefiner::Optimizer::Optimizer(const Snake& snake, const Vec2f& unitDownVec, float factor)
    : m_factor(factor), m_snakeLength(snake) {
  calcFrenetFrames(m_frenetFrames, snake, m_snakeLength, unitDownVec);
}

bool TextLineRefiner::Optimizer::thicknessAdjustment(Snake& snake, const Grid<float>& gradient) {
  const size_t numNodes = snake.nodes.size();

  const float rib_adjustments[] = {0.0f * m_factor, 0.5f * m_factor, -0.5f * m_factor};
  enum { NUM_RIB_ADJUSTMENTS = sizeof(rib_adjustments) / sizeof(rib_adjustments[0]) };

  int bestI = 0;
  int bestJ = 0;
  float bestCost = NumericTraits<float>::max();
  for (int i = 0; i < NUM_RIB_ADJUSTMENTS; ++i) {
    const float headRib = snake.nodes.front().ribHalfLength + rib_adjustments[i];
    if (headRib <= std::numeric_limits<float>::epsilon()) {
      continue;
    }

    for (int j = 0; j < NUM_RIB_ADJUSTMENTS; ++j) {
      const float tailRib = snake.nodes.back().ribHalfLength + rib_adjustments[j];
      if (tailRib <= std::numeric_limits<float>::epsilon()) {
        continue;
      }

      float cost = 0;
      for (size_t nodeIdx = 0; nodeIdx < numNodes; ++nodeIdx) {
        const float t = m_snakeLength.arcLengthFractionAt(nodeIdx);
        const float rib = headRib + t * (tailRib - headRib);
        const Vec2f downNormal(m_frenetFrames[nodeIdx].unitDownNormal);

        SnakeNode node(snake.nodes[nodeIdx]);
        node.ribHalfLength = rib;
        cost += calcExternalEnergy(gradient, node, downNormal);
      }
      if (cost < bestCost) {
        bestCost = cost;
        bestI = i;
        bestJ = j;
      }
    }
  }
  const float headRib = snake.nodes.front().ribHalfLength + rib_adjustments[bestI];
  const float tailRib = snake.nodes.back().ribHalfLength + rib_adjustments[bestJ];
  for (size_t nodeIdx = 0; nodeIdx < numNodes; ++nodeIdx) {
    const float t = m_snakeLength.arcLengthFractionAt(nodeIdx);
    snake.nodes[nodeIdx].ribHalfLength = headRib + t * (tailRib - headRib);
    // Note that we need to recalculate inner ribs even if outer ribs
    // didn't change, as movement of ribs in tangent direction affects
    // interpolation.
  }
  return rib_adjustments[bestI] != 0 || rib_adjustments[bestJ] != 0;
}  // TextLineRefiner::Optimizer::thicknessAdjustment

bool TextLineRefiner::Optimizer::tangentMovement(Snake& snake, const Grid<float>& gradient) {
  const size_t numNodes = snake.nodes.size();
  if (numNodes < 3) {
    return false;
  }

  const float tangent_movements[] = {0.0f * m_factor, 1.0f * m_factor, -1.0f * m_factor};
  enum { NUM_TANGENT_MOVEMENTS = sizeof(tangent_movements) / sizeof(tangent_movements[0]) };

  std::vector<uint32_t> paths;
  std::vector<uint32_t> newPaths;
  std::vector<Step> stepStorage;
  // Note that we don't move the first and the last node in tangent direction.
  paths.push_back(static_cast<unsigned int&&>(stepStorage.size()));
  stepStorage.emplace_back();
  stepStorage.back().prevStepIdx = ~uint32_t(0);
  stepStorage.back().node = snake.nodes.front();
  stepStorage.back().pathCost = 0;

  for (size_t nodeIdx = 1; nodeIdx < numNodes - 1; ++nodeIdx) {
    const Vec2f initialPos(snake.nodes[nodeIdx].center);
    const float rib = snake.nodes[nodeIdx].ribHalfLength;
    const Vec2f unitTangent(m_frenetFrames[nodeIdx].unitTangent);
    const Vec2f downNormal(m_frenetFrames[nodeIdx].unitDownNormal);

    for (float tangentMovement : tangent_movements) {
      Step step;
      step.prevStepIdx = ~uint32_t(0);
      step.node.center = initialPos + tangentMovement * unitTangent;
      step.node.ribHalfLength = rib;
      step.pathCost = NumericTraits<float>::max();

      float baseCost = calcExternalEnergy(gradient, step.node, downNormal);

      if (nodeIdx == numNodes - 2) {
        // Take into account the distance to the last node as well.
        baseCost += calcElasticityEnergy(step.node, snake.nodes.back(), m_snakeLength.avgSegmentLength());
      }

      // Now find the best step for the previous node to combine with.
      for (uint32_t prevStepIdx : paths) {
        const Step& prevStep = stepStorage[prevStepIdx];
        const float cost = baseCost + prevStep.pathCost
                           + calcElasticityEnergy(step.node, prevStep.node, m_snakeLength.avgSegmentLength());

        if (cost < step.pathCost) {
          step.pathCost = cost;
          step.prevStepIdx = prevStepIdx;
        }
      }
      assert(step.prevStepIdx != ~uint32_t(0));
      newPaths.push_back(static_cast<unsigned int&&>(stepStorage.size()));
      stepStorage.push_back(step);
    }
    assert(!newPaths.empty());
    paths.swap(newPaths);
    newPaths.clear();
  }

  // Find the best overall path.
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
  float maxSqdist = 0;
  uint32_t stepIdx = bestPathIdx;
  for (auto nodeIdx = static_cast<int>(numNodes - 2); nodeIdx > 0; --nodeIdx) {
    assert(stepIdx != ~uint32_t(0));
    const Step& step = stepStorage[stepIdx];
    SnakeNode& node = snake.nodes[nodeIdx];

    const float sqdist = (node.center - step.node.center).squaredNorm();
    maxSqdist = std::max<float>(maxSqdist, sqdist);

    node = step.node;
    stepIdx = step.prevStepIdx;
  }
  return maxSqdist > std::numeric_limits<float>::epsilon();
}  // TextLineRefiner::Optimizer::tangentMovement

bool TextLineRefiner::Optimizer::normalMovement(Snake& snake, const Grid<float>& gradient) {
  const size_t numNodes = snake.nodes.size();
  if (numNodes < 3) {
    return false;
  }

  const float normal_movements[] = {0.0f * m_factor, 1.0f * m_factor, -1.0f * m_factor};
  enum { NUM_NORMAL_MOVEMENTS = sizeof(normal_movements) / sizeof(normal_movements[0]) };

  std::vector<uint32_t> paths;
  std::vector<uint32_t> newPaths;
  std::vector<Step> stepStorage;
  // The first two nodes pose a problem for us.  These nodes don't have two predecessors,
  // and therefore we can't take bending into the account.  We could take the followers
  // instead of the ancestors, but then this follower is going to move itself, making
  // our calculations less accurate.  The proper solution is to provide not N but N*N
  // paths to the 3rd node, each path corresponding to a combination of movement of
  // the first and the second node.  That's the approach we are taking here.
  for (float normalMovement : normal_movements) {
    const auto prevStepIdx = static_cast<uint32_t>(stepStorage.size());
    {
      // Movements of the first node.
      const Vec2f downNormal(m_frenetFrames[0].unitDownNormal);
      Step step;
      step.node.center = snake.nodes[0].center + normalMovement * downNormal;
      step.node.ribHalfLength = snake.nodes[0].ribHalfLength;
      step.prevStepIdx = ~uint32_t(0);
      step.pathCost = calcExternalEnergy(gradient, step.node, downNormal);

      stepStorage.push_back(step);
    }

    for (float j : normal_movements) {
      // Movements of the second node.
      const Vec2f downNormal(m_frenetFrames[1].unitDownNormal);

      Step step;
      step.node.center = snake.nodes[1].center + j * downNormal;
      step.node.ribHalfLength = snake.nodes[1].ribHalfLength;
      step.prevStepIdx = prevStepIdx;
      step.pathCost = stepStorage[prevStepIdx].pathCost + calcExternalEnergy(gradient, step.node, downNormal);

      paths.push_back(static_cast<unsigned int&&>(stepStorage.size()));
      stepStorage.push_back(step);
    }
  }

  for (size_t nodeIdx = 2; nodeIdx < numNodes; ++nodeIdx) {
    const SnakeNode& node = snake.nodes[nodeIdx];
    const Vec2f downNormal(m_frenetFrames[nodeIdx].unitDownNormal);

    for (float normalMovement : normal_movements) {
      Step step;
      step.prevStepIdx = ~uint32_t(0);
      step.node.center = node.center + normalMovement * downNormal;
      step.node.ribHalfLength = node.ribHalfLength;
      step.pathCost = NumericTraits<float>::max();

      const float baseCost = calcExternalEnergy(gradient, step.node, downNormal);

      // Now find the best step for the previous node to combine with.
      for (uint32_t prevStepIdx : paths) {
        const Step& prevStep = stepStorage[prevStepIdx];
        const Step& prevPrevStep = stepStorage[prevStep.prevStepIdx];

        const float cost
            = baseCost + prevStep.pathCost + calcBendingEnergy(step.node, prevStep.node, prevPrevStep.node);

        if (cost < step.pathCost) {
          step.pathCost = cost;
          step.prevStepIdx = prevStepIdx;
        }
      }
      assert(step.prevStepIdx != ~uint32_t(0));
      newPaths.push_back(static_cast<unsigned int&&>(stepStorage.size()));
      stepStorage.push_back(step);
    }
    assert(!newPaths.empty());
    paths.swap(newPaths);
    newPaths.clear();
  }

  // Find the best overall path.
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
  float maxSqdist = 0;
  uint32_t stepIdx = bestPathIdx;
  for (auto nodeIdx = static_cast<int>(numNodes - 1); nodeIdx >= 0; --nodeIdx) {
    assert(stepIdx != ~uint32_t(0));
    const Step& step = stepStorage[stepIdx];
    SnakeNode& node = snake.nodes[nodeIdx];

    const float sqdist = (node.center - step.node.center).squaredNorm();
    maxSqdist = std::max<float>(maxSqdist, sqdist);

    node = step.node;
    stepIdx = step.prevStepIdx;
  }
  return maxSqdist > std::numeric_limits<float>::epsilon();
}  // TextLineRefiner::Optimizer::normalMovement

float TextLineRefiner::Optimizer::calcExternalEnergy(const Grid<float>& gradient,
                                                     const SnakeNode& node,
                                                     const Vec2f downNormal) {
  const Vec2f top(node.center - node.ribHalfLength * downNormal);
  const Vec2f bottom(node.center + node.ribHalfLength * downNormal);

  const float topGrad = externalEnergyAt(gradient, top, 0.0f);
  const float bottomGrad = externalEnergyAt(gradient, bottom, 0.0f);

  // Surprisingly, it turns out it's a bad idea to penalize for the opposite
  // sign in the gradient.  Sometimes a snake's edge has to move over the
  // "wrong" gradient ridge before it gets into a good position.
  // Those std::min and std::max prevent such penalties.
  const float topEnergy = m_topExternalWeight * std::min<float>(topGrad, 0.0f);
  const float bottomEnergy = m_bottomExternalWeight * std::max<float>(bottomGrad, 0.0f);

  // Positive gradient indicates the bottom edge and vice versa.
  // Note that negative energies are fine with us - the less the better.
  return topEnergy - bottomEnergy;
}

float TextLineRefiner::Optimizer::calcElasticityEnergy(const SnakeNode& node1, const SnakeNode& node2, float avgDist) {
  const Vec2f vec(node1.center - node2.center);
  const auto vecLen = static_cast<float>(std::sqrt(vec.squaredNorm()));

  if (vecLen < 1.0f) {
    return 1000.0f;  // Penalty for moving too close to another node.
  }

  const auto distDiff = std::fabs(avgDist - vecLen);
  return m_elasticityWeight * (distDiff / avgDist);
}

float TextLineRefiner::Optimizer::calcBendingEnergy(const SnakeNode& node,
                                                    const SnakeNode& prevNode,
                                                    const SnakeNode& prevPrevNode) {
  const Vec2f vec(node.center - prevNode.center);
  const auto vecLen = static_cast<float>(std::sqrt(vec.squaredNorm()));

  if (vecLen < 1.0f) {
    return 1000.0f;  // Penalty for moving too close to another node.
  }

  const Vec2f prevVec(prevNode.center - prevPrevNode.center);
  const auto prevVecLen = static_cast<float>(std::sqrt(prevVec.squaredNorm()));
  if (prevVecLen < 1.0f) {
    return 1000.0f;  // Penalty for moving too close to another node.
  }

  const Vec2f bendVec(vec / vecLen - prevVec / prevVecLen);
  return m_bendingWeight * bendVec.squaredNorm();
}
}  // namespace dewarping
