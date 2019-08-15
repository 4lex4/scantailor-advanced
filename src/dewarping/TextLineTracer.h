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

#ifndef DEWARPING_TEXT_LINE_TRACER_H_
#define DEWARPING_TEXT_LINE_TRACER_H_

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
                    const QRect& content_rect,
                    DistortionModelBuilder& output,
                    const TaskStatus& status,
                    DebugImages* dbg = nullptr);

 private:
  static imageproc::GrayImage downscale(const imageproc::GrayImage& input, const Dpi& dpi);

  static void sanitizeBinaryImage(imageproc::BinaryImage& image, const QRect& content_rect);

  static void extractTextLines(std::list<std::vector<QPointF>>& out,
                               const imageproc::GrayImage& image,
                               const std::pair<QLineF, QLineF>& bounds,
                               DebugImages* dbg);

  static Vec2f calcAvgUnitVector(const std::pair<QLineF, QLineF>& bounds);

  static imageproc::BinaryImage closeWithObstacles(const imageproc::BinaryImage& image,
                                                   const imageproc::BinaryImage& obstacles,
                                                   const QSize& brick);

  static QLineF calcMidLine(const QLineF& line1, const QLineF& line2);

  static void findMidLineSeeds(const imageproc::SEDM& sedm, QLineF mid_line, std::vector<QPoint>& seeds);

  static bool isCurvatureConsistent(const std::vector<QPointF>& polyline);

  static bool isInsideBounds(const QPointF& pt, const QLineF& left_bound, const QLineF& right_bound);

  static void filterShortCurves(std::list<std::vector<QPointF>>& polylines,
                                const QLineF& left_bound,
                                const QLineF& right_bound);

  static void filterOutOfBoundsCurves(std::list<std::vector<QPointF>>& polylines,
                                      const QLineF& left_bound,
                                      const QLineF& right_bound);

  static void filterEdgyCurves(std::list<std::vector<QPointF>>& polylines);

  static QImage visualizeVerticalBounds(const QImage& background, const std::pair<QLineF, QLineF>& bounds);

  static QImage visualizeGradient(const QImage& background, const Grid<float>& grad);

  static QImage visualizeMidLineSeeds(const QImage& background,
                                      const imageproc::BinaryImage& overlay,
                                      std::pair<QLineF, QLineF> bounds,
                                      QLineF mid_line,
                                      const std::vector<QPoint>& seeds);

  static QImage visualizePolylines(const QImage& background,
                                   const std::list<std::vector<QPointF>>& polylines,
                                   const std::pair<QLineF, QLineF>* vert_bounds = nullptr);
};
}  // namespace dewarping
#endif  // ifndef DEWARPING_TEXT_LINE_TRACER_H_
