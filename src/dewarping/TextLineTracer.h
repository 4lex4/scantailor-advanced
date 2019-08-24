// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_DEWARPING_TEXTLINETRACER_H_
#define SCANTAILOR_DEWARPING_TEXTLINETRACER_H_

#include <QLineF>
#include <QPoint>
#include <QPointF>
#include <cstdint>
#include <list>
#include <utility>
#include <vector>
#include "Grid.h"
#include "VecNT.h"

class Dpi;
class QImage;
class QSize;
class QRect;
class TaskStatus;
class DebugImages;

namespace imageproc {
class BinaryImage;
class GrayImage;

class SEDM;
}  // namespace imageproc

namespace dewarping {
class DistortionModelBuilder;

class TextLineTracer {
 public:
  static void trace(const imageproc::GrayImage& input,
                    const Dpi& dpi,
                    const QRect& contentRect,
                    DistortionModelBuilder& output,
                    const TaskStatus& status,
                    DebugImages* dbg = nullptr);

 private:
  static imageproc::GrayImage downscale(const imageproc::GrayImage& input, const Dpi& dpi);

  static void sanitizeBinaryImage(imageproc::BinaryImage& image, const QRect& contentRect);

  static void extractTextLines(std::list<std::vector<QPointF>>& out,
                               const imageproc::GrayImage& image,
                               const std::pair<QLineF, QLineF>& bounds,
                               DebugImages* dbg);

  static Vec2f calcAvgUnitVector(const std::pair<QLineF, QLineF>& bounds);

  static imageproc::BinaryImage closeWithObstacles(const imageproc::BinaryImage& image,
                                                   const imageproc::BinaryImage& obstacles,
                                                   const QSize& brick);

  static QLineF calcMidLine(const QLineF& line1, const QLineF& line2);

  static void findMidLineSeeds(const imageproc::SEDM& sedm, QLineF midLine, std::vector<QPoint>& seeds);

  static bool isCurvatureConsistent(const std::vector<QPointF>& polyline);

  static bool isInsideBounds(const QPointF& pt, const QLineF& leftBound, const QLineF& rightBound);

  static void filterShortCurves(std::list<std::vector<QPointF>>& polylines,
                                const QLineF& leftBound,
                                const QLineF& rightBound);

  static void filterOutOfBoundsCurves(std::list<std::vector<QPointF>>& polylines,
                                      const QLineF& leftBound,
                                      const QLineF& rightBound);

  static void filterEdgyCurves(std::list<std::vector<QPointF>>& polylines);

  static QImage visualizeVerticalBounds(const QImage& background, const std::pair<QLineF, QLineF>& bounds);

  static QImage visualizeGradient(const QImage& background, const Grid<float>& grad);

  static QImage visualizeMidLineSeeds(const QImage& background,
                                      const imageproc::BinaryImage& overlay,
                                      std::pair<QLineF, QLineF> bounds,
                                      QLineF midLine,
                                      const std::vector<QPoint>& seeds);

  static QImage visualizePolylines(const QImage& background,
                                   const std::list<std::vector<QPointF>>& polylines,
                                   const std::pair<QLineF, QLineF>* vertBounds = nullptr);
};
}  // namespace dewarping
#endif  // ifndef SCANTAILOR_DEWARPING_TEXTLINETRACER_H_
