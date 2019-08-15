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

#ifndef DEWARPING_DISTORTION_MODEL_BUILDER_H_
#define DEWARPING_DISTORTION_MODEL_BUILDER_H_

#include <QLineF>
#include <QPointF>
#include <QTransform>
#include <deque>
#include <utility>
#include <vector>
#include "VecNT.h"

class QImage;
class DebugImages;
class XSpline;

namespace dewarping {
class DistortionModel;

class DistortionModelBuilder {
  // Member-wise copying is OK.
 public:
  /**
   * \brief Constructor.
   *
   * \param down_direction A vector pointing approximately downwards in terms of content.
   *        The vector can't be zero-length.
   */
  explicit DistortionModelBuilder(const Vec2d& down_direction);

  /**
   * \brief Set the vertical content boundaries.
   *
   * Note that we are setting lines, not line segments, so endpoint
   * positions along the line don't really matter.  It also doesn't
   * matter which one is the left bound and which one is the right one.
   */
  void setVerticalBounds(const QLineF& bound1, const QLineF& bound2);

  /**
   * \brief Returns the current vertical bounds.
   *
   * It's not specified which one is the left and which one is the right bound.
   */
  std::pair<QLineF, QLineF> verticalBounds() const;

  /**
   * \brief Add a curve that's meant to become straight and horizontal after dewarping.
   *
   * The curve doesn't have to touch or intersect the vertical bounds, although
   * long curves are preferable.  The minimum number of curves to build a distortion
   * model is 2, although that doesn't guarantee successful model construction.
   * The more apart the curves are, the better.
   */
  void addHorizontalCurve(const std::vector<QPointF>& polyline);

  /**
   * \brief Applies an affine transformation to the internal representation.
   */
  void transform(const QTransform& xform);

  /**
   * \brief Tries to build a distortion model based on information provided so far.
   *
   * \return A DistortionModel that may be invalid.
   * \see DistortionModel::isValid()
   */
  DistortionModel tryBuildModel(DebugImages* dbg = nullptr, const QImage* dbg_background = nullptr) const;

 private:
  struct TracedCurve;
  struct RansacModel;

  class RansacAlgo;
  class BadCurve;

  TracedCurve polylineToCurve(const std::vector<QPointF>& polyline) const;

  static Vec2d centroid(const std::vector<QPointF>& polyline);

  std::pair<QLineF, QLineF> frontBackBounds(const std::vector<QPointF>& polyline) const;

  static std::vector<QPointF> maybeTrimPolyline(const std::vector<QPointF>& polyline,
                                                const std::pair<QLineF, QLineF>& bounds);

  static bool maybeTrimFront(std::deque<QPointF>& polyline, const QLineF& bound);

  static bool maybeTrimBack(std::deque<QPointF>& polyline, const QLineF& bound);

  static void intersectFront(std::deque<QPointF>& polyline, const QLineF& bound);

  static void intersectBack(std::deque<QPointF>& polyline, const QLineF& bound);

  static XSpline fitExtendedSpline(const std::vector<QPointF>& polyline,
                                   const Vec2d& centroid,
                                   const std::pair<QLineF, QLineF>& bounds);

  QImage visualizeTrimmedPolylines(const QImage& background, const std::vector<TracedCurve>& curves) const;

  QImage visualizeModel(const QImage& background,
                        const std::vector<TracedCurve>& curves,
                        const RansacModel& model) const;

  Vec2d m_downDirection;
  Vec2d m_rightDirection;
  QLineF m_bound1;  // It's not specified which one is left
  QLineF m_bound2;  // and which one is right.

  /** These go left to right in terms of content. */
  std::deque<std::vector<QPointF>> m_ltrPolylines;
};
}  // namespace dewarping
#endif  // ifndef DEWARPING_DISTORTION_MODEL_BUILDER_H_
