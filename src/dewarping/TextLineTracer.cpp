// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "TextLineTracer.h"

#include <Binarize.h>
#include <BinaryImage.h>
#include <Constants.h>
#include <GaussBlur.h>
#include <Grayscale.h>
#include <LocalMinMaxGeneric.h>
#include <Morphology.h>
#include <RasterOp.h>
#include <RasterOpGeneric.h>
#include <SEDM.h>
#include <Scale.h>
#include <SeedFill.h>
#include <Sobel.h>

#include <QPainter>
#include <boost/foreach.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/lambda/if.hpp>
#include <boost/lambda/lambda.hpp>
#include <cmath>

#include "DebugImages.h"
#include "DetectVertContentBounds.h"
#include "DistortionModel.h"
#include "DistortionModelBuilder.h"
#include "GridLineTraverser.h"
#include "LineBoundedByRect.h"
#include "NumericTraits.h"
#include "TaskStatus.h"
#include "TextLineRefiner.h"
#include "ToLineProjector.h"
#include "TowardsLineTracer.h"

using namespace imageproc;

namespace dewarping {
void TextLineTracer::trace(const GrayImage& input,
                           const Dpi& dpi,
                           const QRect& contentRect,
                           DistortionModelBuilder& output,
                           const TaskStatus& status,
                           DebugImages* dbg) {
  GrayImage downscaled(downscale(input, dpi));
  if (dbg) {
    dbg->add(downscaled, "downscaled");
  }

  const int downscaledWidth = downscaled.width();
  const int downscaledHeight = downscaled.height();

  const double downscaleXFactor = double(downscaledWidth) / input.width();
  const double downscaleYFactor = double(downscaledHeight) / input.height();
  QTransform toOrig;
  toOrig.scale(1.0 / downscaleXFactor, 1.0 / downscaleYFactor);

  const QRect downscaledContentRect(toOrig.inverted().mapRect(contentRect));
  const Dpi downscaledDpi(qRound(dpi.horizontal() * downscaleXFactor), qRound(dpi.vertical() * downscaleYFactor));

  BinaryImage binarized(binarizeWolf(downscaled, QSize(31, 31)));
  if (dbg) {
    dbg->add(binarized, "binarized");
  }
  // detectVertContentBounds() is sensitive to clutter and speckles, so let's try to remove it.
  sanitizeBinaryImage(binarized, downscaledContentRect);
  if (dbg) {
    dbg->add(binarized, "sanitized");
  }

  std::pair<QLineF, QLineF> vertBounds(detectVertContentBounds(binarized, dbg));
  if (dbg) {
    dbg->add(visualizeVerticalBounds(binarized.toQImage(), vertBounds), "vertBounds");
  }

  std::list<std::vector<QPointF>> polylines;
  extractTextLines(polylines, stretchGrayRange(downscaled), vertBounds, dbg);
  if (dbg) {
    dbg->add(visualizePolylines(downscaled, polylines), "traced");
  }

  filterShortCurves(polylines, vertBounds.first, vertBounds.second);
  filterOutOfBoundsCurves(polylines, vertBounds.first, vertBounds.second);
  if (dbg) {
    dbg->add(visualizePolylines(downscaled, polylines), "filtered1");
  }

  Vec2f unitDownVector(calcAvgUnitVector(vertBounds));
  unitDownVector /= std::sqrt(unitDownVector.squaredNorm());
  if (unitDownVector[1] < 0) {
    unitDownVector = -unitDownVector;
  }
  TextLineRefiner refiner(downscaled, Dpi(200, 200), unitDownVector);
  refiner.refine(polylines, /*iterations=*/100, dbg);

  filterEdgyCurves(polylines);
  if (dbg) {
    dbg->add(visualizePolylines(downscaled, polylines), "filtered2");
  }


  // Transform back to original coordinates and output.

  vertBounds.first = toOrig.map(vertBounds.first);
  vertBounds.second = toOrig.map(vertBounds.second);
  output.setVerticalBounds(vertBounds.first, vertBounds.second);

  for (std::vector<QPointF>& polyline : polylines) {
    for (QPointF& pt : polyline) {
      pt = toOrig.map(pt);
    }
    output.addHorizontalCurve(polyline);
  }
}  // TextLineTracer::trace

GrayImage TextLineTracer::downscale(const GrayImage& input, const Dpi& dpi) {
  // Downscale to 200 DPI.
  QSize downscaledSize(input.size());
  if ((dpi.horizontal() < 180) || (dpi.horizontal() > 220) || (dpi.vertical() < 180) || (dpi.vertical() > 220)) {
    downscaledSize.setWidth(std::max<int>(1, input.width() * 200 / dpi.horizontal()));
    downscaledSize.setHeight(std::max<int>(1, input.height() * 200 / dpi.vertical()));
  }
  return scaleToGray(input, downscaledSize);
}

void TextLineTracer::sanitizeBinaryImage(BinaryImage& image, const QRect& contentRect) {
  // Kill connected components touching the borders.
  BinaryImage seed(image.size(), WHITE);
  seed.fillExcept(seed.rect().adjusted(1, 1, -1, -1), BLACK);

  BinaryImage touchingBorder(seedFill(seed.release(), image, CONN8));
  rasterOp<RopSubtract<RopDst, RopSrc>>(image, touchingBorder.release());

  // Poor man's despeckle.
  BinaryImage contentSeeds(openBrick(image, QSize(2, 3), WHITE));
  rasterOp<RopOr<RopSrc, RopDst>>(contentSeeds, openBrick(image, QSize(3, 2), WHITE));
  image = seedFill(contentSeeds.release(), image, CONN8);
  // Clear margins.
  image.fillExcept(contentRect, WHITE);
}

/**
 * Returns false if the curve contains both significant convexities and concavities.
 */
bool TextLineTracer::isCurvatureConsistent(const std::vector<QPointF>& polyline) {
  const size_t numNodes = polyline.size();

  if (numNodes <= 1) {
    // Even though we can't say anything about curvature in this case,
    // we don't like such gegenerate curves, so we reject them.
    return false;
  } else if (numNodes == 2) {
    // These are fine.
    return true;
  }
  // Threshold angle between a polyline segment and a normal to the previous one.
  const auto cosThreshold = static_cast<float>(std::cos((90.0f - 6.0f) * constants::DEG2RAD));
  const float cosSqThreshold = cosThreshold * cosThreshold;
  bool significantPositive = false;
  bool significantNegative = false;

  Vec2f prevNormal(polyline[1] - polyline[0]);
  std::swap(prevNormal[0], prevNormal[1]);
  prevNormal[0] = -prevNormal[0];
  float prevNormalSqlen = prevNormal.squaredNorm();

  for (size_t i = 1; i < numNodes - 1; ++i) {
    const Vec2f nextSegment(polyline[i + 1] - polyline[i]);
    const float nextSegmentSqlen = nextSegment.squaredNorm();

    float cosSq = 0;
    const float sqlenMult = prevNormalSqlen * nextSegmentSqlen;
    if (sqlenMult > std::numeric_limits<float>::epsilon()) {
      const float dot = prevNormal.dot(nextSegment);
      cosSq = std::fabs(dot) * dot / sqlenMult;
    }

    if (std::fabs(cosSq) >= cosSqThreshold) {
      if (cosSq > 0) {
        significantPositive = true;
      } else {
        significantNegative = true;
      }
    }

    prevNormal[0] = -nextSegment[1];
    prevNormal[1] = nextSegment[0];
    prevNormalSqlen = nextSegmentSqlen;
  }
  return !(significantPositive && significantNegative);
}  // TextLineTracer::isCurvatureConsistent

bool TextLineTracer::isInsideBounds(const QPointF& pt, const QLineF& leftBound, const QLineF& rightBound) {
  QPointF leftNormalInside(leftBound.normalVector().p2() - leftBound.p1());
  if (leftNormalInside.x() < 0) {
    leftNormalInside = -leftNormalInside;
  }
  const QPointF leftVec(pt - leftBound.p1());
  if (leftNormalInside.x() * leftVec.x() + leftNormalInside.y() * leftVec.y() < 0) {
    return false;
  }

  QPointF rightNormalInside(rightBound.normalVector().p2() - rightBound.p1());
  if (rightNormalInside.x() > 0) {
    rightNormalInside = -rightNormalInside;
  }
  const QPointF rightVec(pt - rightBound.p1());
  if (rightNormalInside.x() * rightVec.x() + rightNormalInside.y() * rightVec.y() < 0) {
    return false;
  }
  return true;
}

void TextLineTracer::filterShortCurves(std::list<std::vector<QPointF>>& polylines,
                                       const QLineF& leftBound,
                                       const QLineF& rightBound) {
  const ToLineProjector proj1(leftBound);
  const ToLineProjector proj2(rightBound);

  auto it(polylines.begin());
  const auto end(polylines.end());
  while (it != end) {
    assert(!it->empty());
    const QPointF front(it->front());
    const QPointF back(it->back());
    const double frontProjLen = proj1.projectionDist(front);
    const double backProjLen = proj2.projectionDist(back);
    const double chordLen = QLineF(front, back).length();

    if (frontProjLen + backProjLen > 0.3 * chordLen) {
      polylines.erase(it++);
    } else {
      ++it;
    }
  }
}

void TextLineTracer::filterOutOfBoundsCurves(std::list<std::vector<QPointF>>& polylines,
                                             const QLineF& leftBound,
                                             const QLineF& rightBound) {
  auto it(polylines.begin());
  const auto end(polylines.end());
  while (it != end) {
    if (!isInsideBounds(it->front(), leftBound, rightBound) && !isInsideBounds(it->back(), leftBound, rightBound)) {
      polylines.erase(it++);
    } else {
      ++it;
    }
  }
}

void TextLineTracer::filterEdgyCurves(std::list<std::vector<QPointF>>& polylines) {
  auto it(polylines.begin());
  const auto end(polylines.end());
  while (it != end) {
    if (!isCurvatureConsistent(*it)) {
      polylines.erase(it++);
    } else {
      ++it;
    }
  }
}

void TextLineTracer::extractTextLines(std::list<std::vector<QPointF>>& out,
                                      const imageproc::GrayImage& image,
                                      const std::pair<QLineF, QLineF>& bounds,
                                      DebugImages* dbg) {
  using namespace boost::lambda;

  const int width = image.width();
  const int height = image.height();
  const QSize size(image.size());
  const Vec2f direction(calcAvgUnitVector(bounds));
  Grid<float> mainGrid(image.width(), image.height(), 0);
  Grid<float> auxGrid(image.width(), image.height(), 0);

  const float downscale = 1.0f / (255.0f * 8.0f);
  horizontalSobel<float>(width, height, image.data(), image.stride(), _1 * downscale, auxGrid.data(), auxGrid.stride(),
                         _1 = _2, _1, mainGrid.data(), mainGrid.stride(), _1 = _2);
  verticalSobel<float>(width, height, image.data(), image.stride(), _1 * downscale, auxGrid.data(), auxGrid.stride(),
                       _1 = _2, _1, mainGrid.data(), mainGrid.stride(), _1 = _1 * direction[0] + _2 * direction[1]);
  if (dbg) {
    dbg->add(visualizeGradient(image, mainGrid), "first_dir_deriv");
  }

  gaussBlurGeneric(size, 6.0f, 6.0f, mainGrid.data(), mainGrid.stride(), _1, mainGrid.data(), mainGrid.stride(),
                   _1 = _2);
  if (dbg) {
    dbg->add(visualizeGradient(image, mainGrid), "first_dir_deriv_blurred");
  }

  horizontalSobel<float>(width, height, mainGrid.data(), mainGrid.stride(), _1, auxGrid.data(), auxGrid.stride(),
                         _1 = _2, _1, auxGrid.data(), auxGrid.stride(), _1 = _2);
  verticalSobel<float>(width, height, mainGrid.data(), mainGrid.stride(), _1, mainGrid.data(), mainGrid.stride(),
                       _1 = _2, _1, mainGrid.data(), mainGrid.stride(), _1 = _2);
  rasterOpGeneric(auxGrid.data(), auxGrid.stride(), size, mainGrid.data(), mainGrid.stride(),
                  _2 = _1 * direction[0] + _2 * direction[1]);
  if (dbg) {
    dbg->add(visualizeGradient(image, mainGrid), "second_dir_deriv");
  }

  float max = 0;
  rasterOpGeneric(mainGrid.data(), mainGrid.stride(), size, if_then(_1 > var(max), var(max) = _1));
  const float threshold = max * 15.0f / 255.0f;

  BinaryImage initialBinarization(image.size());
  rasterOpGeneric(initialBinarization, mainGrid.data(), mainGrid.stride(),
                  if_then_else(_2 > threshold, _1 = uint32_t(1), _1 = uint32_t(0)));
  if (dbg) {
    dbg->add(initialBinarization, "initialBinarization");
  }

  rasterOpGeneric(mainGrid.data(), mainGrid.stride(), size, auxGrid.data(), auxGrid.stride(),
                  _2 = bind((float (*)(float)) & std::fabs, _1));
  if (dbg) {
    dbg->add(visualizeGradient(image, auxGrid), "abs");
  }

  gaussBlurGeneric(size, 12.0f, 12.0f, auxGrid.data(), auxGrid.stride(), _1, auxGrid.data(), auxGrid.stride(), _1 = _2);
  if (dbg) {
    dbg->add(visualizeGradient(image, auxGrid), "blurred");
  }

  rasterOpGeneric(mainGrid.data(), mainGrid.stride(), size, auxGrid.data(), auxGrid.stride(),
                  _2 += _1 - bind((float (*)(float)) & std::fabs, _1));
  if (dbg) {
    dbg->add(visualizeGradient(image, auxGrid), "+= diff");
  }

  BinaryImage postBinarization(image.size());
  rasterOpGeneric(postBinarization, auxGrid.data(), auxGrid.stride(),
                  if_then_else(_2 > threshold, _1 = uint32_t(1), _1 = uint32_t(0)));
  if (dbg) {
    dbg->add(postBinarization, "postBinarization");
  }

  BinaryImage obstacles(image.size());
  rasterOpGeneric(obstacles, auxGrid.data(), auxGrid.stride(),
                  if_then_else(_2 < -threshold, _1 = uint32_t(1), _1 = uint32_t(0)));
  if (dbg) {
    dbg->add(obstacles, "obstacles");
  }

  Grid<float>().swap(auxGrid);  // Save memory.
  initialBinarization = closeWithObstacles(initialBinarization, obstacles, QSize(21, 21));
  if (dbg) {
    dbg->add(initialBinarization, "initial_closed");
  }

  obstacles.release();  // Save memory.
  rasterOp<RopAnd<RopDst, RopSrc>>(postBinarization, initialBinarization);
  if (dbg) {
    dbg->add(postBinarization, "post &&= initial");
  }

  initialBinarization.release();  // Save memory.
  const SEDM sedm(postBinarization);

  std::vector<QPoint> seeds;
  QLineF midLine(calcMidLine(bounds.first, bounds.second));
  findMidLineSeeds(sedm, midLine, seeds);
  if (dbg) {
    dbg->add(visualizeMidLineSeeds(image, postBinarization, bounds, midLine, seeds), "seeds");
  }

  postBinarization.release();  // Save memory.
  for (const QPoint seed : seeds) {
    std::vector<QPointF> polyline;

    {
      TowardsLineTracer tracer(&sedm, &mainGrid, bounds.first, seed);
      while (const QPoint* pt = tracer.trace(10.0f)) {
        polyline.emplace_back(*pt);
      }
      std::reverse(polyline.begin(), polyline.end());
    }

    polyline.emplace_back(seed);

    {
      TowardsLineTracer tracer(&sedm, &mainGrid, bounds.second, seed);
      while (const QPoint* pt = tracer.trace(10.0f)) {
        polyline.emplace_back(*pt);
      }
    }

    out.emplace_back();
    out.back().swap(polyline);
  }
}  // TextLineTracer::extractTextLines

Vec2f TextLineTracer::calcAvgUnitVector(const std::pair<QLineF, QLineF>& bounds) {
  Vec2f v1(bounds.first.p2() - bounds.first.p1());
  v1 /= std::sqrt(v1.squaredNorm());

  Vec2f v2(bounds.second.p2() - bounds.second.p1());
  v2 /= std::sqrt(v2.squaredNorm());

  Vec2f v3(v1 + v2);
  v3 /= std::sqrt(v3.squaredNorm());
  return v3;
}

BinaryImage TextLineTracer::closeWithObstacles(const BinaryImage& image,
                                               const BinaryImage& obstacles,
                                               const QSize& brick) {
  BinaryImage mask(closeBrick(image, brick));
  rasterOp<RopSubtract<RopDst, RopSrc>>(mask, obstacles);
  return seedFill(image, mask, CONN4);
}

void TextLineTracer::findMidLineSeeds(const SEDM& sedm, QLineF midLine, std::vector<QPoint>& seeds) {
  lineBoundedByRect(midLine, QRect(QPoint(0, 0), sedm.size()).adjusted(0, 0, -1, -1));

  const uint32_t* sedmData = sedm.data();
  const int sedmStride = sedm.stride();

  QPoint prevPt;
  int32_t prevLevel = 0;
  int dir = 1;  // Distance growing.
  GridLineTraverser traverser(midLine);
  while (traverser.hasNext()) {
    const QPoint pt(traverser.next());
    const int32_t level = sedmData[pt.y() * sedmStride + pt.x()];
    if ((level - prevLevel) * dir < 0) {
      // Direction changed.
      if (dir > 0) {
        seeds.push_back(prevPt);
      }
      dir *= -1;
    }

    prevPt = pt;
    prevLevel = level;
  }
}

QLineF TextLineTracer::calcMidLine(const QLineF& line1, const QLineF& line2) {
  QPointF intersection;
#if QT_VERSION_MAJOR == 5 and QT_VERSION_MINOR < 14
  auto is = line1.intersect(line2, &intersection);
#else
  auto is = line1.intersects(line2, &intersection);
#endif
  if (is == QLineF::NoIntersection) {
    // Lines are parallel.
    const QPointF p1(line2.p1());
    const QPointF p2(ToLineProjector(line1).projectionPoint(p1));
    const QPointF origin(0.5 * (p1 + p2));
    const QPointF vector(line2.p2() - line2.p1());
    return QLineF(origin, origin + vector);
  } else {
    // Lines do intersect.
    Vec2d v1(line1.p2() - line1.p1());
    Vec2d v2(line2.p2() - line2.p1());
    v1 /= std::sqrt(v1.squaredNorm());
    v2 /= std::sqrt(v2.squaredNorm());
    return QLineF(intersection, intersection + 0.5 * (v1 + v2));
  }
}

QImage TextLineTracer::visualizeVerticalBounds(const QImage& background, const std::pair<QLineF, QLineF>& bounds) {
  QImage canvas(background.convertToFormat(QImage::Format_RGB32));

  QPainter painter(&canvas);
  painter.setRenderHint(QPainter::Antialiasing);
  QPen pen(Qt::blue);
  pen.setWidthF(2.0);
  painter.setPen(pen);
  painter.setOpacity(0.7);

  painter.drawLine(bounds.first);
  painter.drawLine(bounds.second);
  return canvas;
}

QImage TextLineTracer::visualizeGradient(const QImage& background, const Grid<float>& grad) {
  const int width = grad.width();
  const int height = grad.height();
  const int gradStride = grad.stride();
  // First let's find the maximum and minimum values.
  float minValue = NumericTraits<float>::max();
  float maxValue = NumericTraits<float>::min();

  const float* gradLine = grad.data();
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      const float value = gradLine[x];
      if (value < minValue) {
        minValue = value;
      } else if (value > maxValue) {
        maxValue = value;
      }
    }
    gradLine += gradStride;
  }

  float scale = std::max(maxValue, -minValue);
  if (scale > std::numeric_limits<float>::epsilon()) {
    scale = 255.0f / scale;
  }

  QImage overlay(width, height, QImage::Format_ARGB32_Premultiplied);
  auto* overlayLine = (uint32_t*) overlay.bits();
  const int overlayStride = overlay.bytesPerLine() / 4;

  gradLine = grad.data();
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      const float value = gradLine[x] * scale;
      const int magnitude = qBound(0, static_cast<const int&>(std::round(std::fabs(value))), 255);
      if (value < 0) {
        overlayLine[x] = qRgba(0, 0, magnitude, magnitude);
      } else {
        overlayLine[x] = qRgba(magnitude, 0, 0, magnitude);
      }
    }
    gradLine += gradStride;
    overlayLine += overlayStride;
  }

  QImage canvas(background.convertToFormat(QImage::Format_ARGB32_Premultiplied));
  QPainter painter(&canvas);
  painter.drawImage(0, 0, overlay);
  return canvas;
}  // TextLineTracer::visualizeGradient

QImage TextLineTracer::visualizeMidLineSeeds(const QImage& background,
                                             const BinaryImage& overlay,
                                             std::pair<QLineF, QLineF> bounds,
                                             QLineF midLine,
                                             const std::vector<QPoint>& seeds) {
  QImage canvas(background.convertToFormat(QImage::Format_ARGB32_Premultiplied));
  QPainter painter(&canvas);
  painter.setRenderHint(QPainter::Antialiasing);

  painter.drawImage(QPoint(0, 0), overlay.toAlphaMask(QColor(0xff, 0x00, 0x00, 120)));

  lineBoundedByRect(bounds.first, background.rect());
  lineBoundedByRect(bounds.second, background.rect());
  lineBoundedByRect(midLine, background.rect());

  QPen pen(QColor(0x00, 0x00, 0xff, 180));
  pen.setWidthF(5.0);
  painter.setPen(pen);
  painter.drawLine(bounds.first);
  painter.drawLine(bounds.second);

  pen.setColor(QColor(0x00, 0xff, 0x00, 180));
  painter.setPen(pen);
  painter.drawLine(midLine);

  painter.setPen(Qt::NoPen);
  painter.setBrush(QColor(0x2d, 0x00, 0x6d, 255));
  QRectF rect(0, 0, 7, 7);
  for (const QPoint pt : seeds) {
    rect.moveCenter(pt + QPointF(0.5, 0.5));
    painter.drawEllipse(rect);
  }
  return canvas;
}  // TextLineTracer::visualizeMidLineSeeds

QImage TextLineTracer::visualizePolylines(const QImage& background,
                                          const std::list<std::vector<QPointF>>& polylines,
                                          const std::pair<QLineF, QLineF>* vertBounds) {
  QImage canvas(background.convertToFormat(QImage::Format_ARGB32_Premultiplied));
  QPainter painter(&canvas);
  painter.setRenderHint(QPainter::Antialiasing);
  QPen pen(Qt::blue);
  pen.setWidthF(3.0);
  painter.setPen(pen);

  for (const std::vector<QPointF>& polyline : polylines) {
    if (!polyline.empty()) {
      painter.drawPolyline(&polyline[0], static_cast<int>(polyline.size()));
    }
  }

  if (vertBounds) {
    painter.drawLine(vertBounds->first);
    painter.drawLine(vertBounds->second);
  }
  return canvas;
}
}  // namespace dewarping
