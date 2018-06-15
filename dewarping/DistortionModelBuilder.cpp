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

#include "DistortionModelBuilder.h"
#include <QDebug>
#include <QImage>
#include <QPainter>
#include <boost/foreach.hpp>
#include "CylindricalSurfaceDewarper.h"
#include "DebugImages.h"
#include "DistortionModel.h"
#include "LineBoundedByRect.h"
#include "SidesOfLine.h"
#include "ToLineProjector.h"
#include "spfit/ConstraintSet.h"
#include "spfit/FrenetFrame.h"
#include "spfit/LinearForceBalancer.h"
#include "spfit/PolylineModelShape.h"
#include "spfit/SplineFitter.h"
#include "spfit/SqDistApproximant.h"

using namespace imageproc;

namespace dewarping {
struct DistortionModelBuilder::TracedCurve {
  std::vector<QPointF> trimmedPolyline;   // Both are left to right.
  std::vector<QPointF> extendedPolyline;  //
  XSpline extendedSpline;
  double order;  // Lesser values correspond to upper curves.

  TracedCurve(const std::vector<QPointF>& trimmed_polyline, const XSpline& extended_spline, double ord)
      : trimmedPolyline(trimmed_polyline),
        extendedPolyline(extended_spline.toPolyline()),
        extendedSpline(extended_spline),
        order(ord) {}

  bool operator<(const TracedCurve& rhs) const { return order < rhs.order; }
};

struct DistortionModelBuilder::RansacModel {
  const TracedCurve* topCurve;
  const TracedCurve* bottomCurve;
  double totalError;

  RansacModel() : topCurve(nullptr), bottomCurve(nullptr), totalError(NumericTraits<double>::max()) {}

  bool isValid() const { return topCurve && bottomCurve; }
};

class DistortionModelBuilder::RansacAlgo {
 public:
  explicit RansacAlgo(const std::vector<TracedCurve>& all_curves) : m_rAllCurves(all_curves) {}

  void buildAndAssessModel(const TracedCurve* top_curve, const TracedCurve* bottom_curve);

  RansacModel& bestModel() { return m_bestModel; }

  const RansacModel& bestModel() const { return m_bestModel; }

 private:
  double calcReferenceHeight(const CylindricalSurfaceDewarper& dewarper, const QPointF& loc);

  RansacModel m_bestModel;
  const std::vector<TracedCurve>& m_rAllCurves;
};


class DistortionModelBuilder::BadCurve : public std::exception {
 public:
  const char* what() const throw() override { return "Bad curve"; }
};


DistortionModelBuilder::DistortionModelBuilder(const Vec2d& down_direction)
    : m_downDirection(down_direction), m_rightDirection(down_direction[1], -down_direction[0]) {
  assert(down_direction.squaredNorm() > 0);
}

void DistortionModelBuilder::setVerticalBounds(const QLineF& bound1, const QLineF& bound2) {
  m_bound1 = bound1;
  m_bound2 = bound2;
}

std::pair<QLineF, QLineF> DistortionModelBuilder::verticalBounds() const {
  return std::pair<QLineF, QLineF>(m_bound1, m_bound2);
}

void DistortionModelBuilder::addHorizontalCurve(const std::vector<QPointF>& polyline) {
  if (polyline.size() < 2) {
    return;
  }

  if (Vec2d(polyline.back() - polyline.front()).dot(m_rightDirection) > 0) {
    m_ltrPolylines.push_back(polyline);
  } else {
    m_ltrPolylines.emplace_back(polyline.rbegin(), polyline.rend());
  }
}

void DistortionModelBuilder::transform(const QTransform& xform) {
  assert(xform.isAffine());

  const QLineF down_line(xform.map(QLineF(QPointF(0, 0), m_downDirection)));
  const QLineF right_line(xform.map(QLineF(QPointF(0, 0), m_rightDirection)));

  m_downDirection = down_line.p2() - down_line.p1();
  m_rightDirection = right_line.p2() - right_line.p1();
  m_bound1 = xform.map(m_bound1);
  m_bound2 = xform.map(m_bound2);

  for (std::vector<QPointF>& polyline : m_ltrPolylines) {
    for (QPointF& pt : polyline) {
      pt = xform.map(pt);
    }
  }
}

DistortionModel DistortionModelBuilder::tryBuildModel(DebugImages* dbg, const QImage* dbg_background) const {
  auto num_curves = static_cast<int>(m_ltrPolylines.size());

  if ((num_curves < 2) || (m_bound1.p1() == m_bound1.p2()) || (m_bound2.p1() == m_bound2.p2())) {
    return DistortionModel();
  }

  std::vector<TracedCurve> ordered_curves;
  ordered_curves.reserve(num_curves);

  for (const std::vector<QPointF>& polyline : m_ltrPolylines) {
    try {
      ordered_curves.push_back(polylineToCurve(polyline));
    } catch (const BadCurve&) {
      // Just skip it.
    }
  }
  num_curves = static_cast<int>(ordered_curves.size());
  if (num_curves == 0) {
    return DistortionModel();
  }
  // if (num_curves < 2) {
  // return DistortionModel();
  // }
  std::sort(ordered_curves.begin(), ordered_curves.end());

  // Select the best pair using RANSAC.
  RansacAlgo ransac(ordered_curves);

  // First let's try to combine each of the 3 top-most lines
  // with each of the 3 bottom-most ones.
  for (int i = 0; i < std::min<int>(3, num_curves); ++i) {
    for (int j = std::max<int>(0, num_curves - 3); j < num_curves; ++j) {
      if (i < j) {
        ransac.buildAndAssessModel(&ordered_curves[i], &ordered_curves[j]);
      }
    }
  }
  // Continue by throwing in some random pairs of lines.
  qsrand(0);  // Repeatablity is important.
  int random_pairs_remaining = 10;
  while (random_pairs_remaining-- > 0) {
    int i = qrand() % num_curves;
    int j = qrand() % num_curves;
    if (i > j) {
      std::swap(i, j);
    }
    if (i < j) {
      ransac.buildAndAssessModel(&ordered_curves[i], &ordered_curves[j]);
    }
  }

  if (dbg && dbg_background) {
    dbg->add(visualizeTrimmedPolylines(*dbg_background, ordered_curves), "trimmed_polylines");
    dbg->add(visualizeModel(*dbg_background, ordered_curves, ransac.bestModel()), "distortion_model");
  }

  DistortionModel model;
  if (ransac.bestModel().isValid()) {
    model.setTopCurve(Curve(ransac.bestModel().topCurve->extendedPolyline));
    model.setBottomCurve(Curve(ransac.bestModel().bottomCurve->extendedPolyline));
  }

  return model;
}  // DistortionModelBuilder::tryBuildModel

DistortionModelBuilder::TracedCurve DistortionModelBuilder::polylineToCurve(
    const std::vector<QPointF>& polyline) const {
  const std::pair<QLineF, QLineF> bounds(frontBackBounds(polyline));

  // Trim the polyline if necessary.
  const std::vector<QPointF> trimmed_polyline(maybeTrimPolyline(polyline, bounds));

  const Vec2d centroid(this->centroid(polyline));

  // Fit the polyline to a spline, extending it to bounds at the same time.
  const XSpline extended_spline(fitExtendedSpline(trimmed_polyline, centroid, bounds));

  const double order = centroid.dot(m_downDirection);

  return TracedCurve(trimmed_polyline, extended_spline, order);
}

Vec2d DistortionModelBuilder::centroid(const std::vector<QPointF>& polyline) {
  const auto num_points = static_cast<const int>(polyline.size());
  if (num_points == 0) {
    return Vec2d();
  } else if (num_points == 1) {
    return Vec2d(polyline.front());
  }

  Vec2d accum(0, 0);
  double total_weight = 0;

  for (int i = 1; i < num_points; ++i) {
    const QLineF segment(polyline[i - 1], polyline[i]);
    const Vec2d center(0.5 * (segment.p1() + segment.p2()));
    const double weight = segment.length();
    accum += center * weight;
    total_weight += weight;
  }

  if (total_weight < 1e-06) {
    return Vec2d(polyline.front());
  } else {
    return accum / total_weight;
  }
}

/**
 * \brief Returns bounds ordered according to the direction of \p polyline.
 *
 * The first and second bounds will correspond to polyline.front() and polyline.back()
 * respectively.
 */
std::pair<QLineF, QLineF> DistortionModelBuilder::frontBackBounds(const std::vector<QPointF>& polyline) const {
  assert(!polyline.empty());

  const ToLineProjector proj1(m_bound1);
  const ToLineProjector proj2(m_bound2);
  if (proj1.projectionDist(polyline.front()) + proj2.projectionDist(polyline.back())
      < proj1.projectionDist(polyline.back()) + proj2.projectionDist(polyline.front())) {
    return std::pair<QLineF, QLineF>(m_bound1, m_bound2);
  } else {
    return std::pair<QLineF, QLineF>(m_bound2, m_bound1);
  }
}

std::vector<QPointF> DistortionModelBuilder::maybeTrimPolyline(const std::vector<QPointF>& polyline,
                                                               const std::pair<QLineF, QLineF>& bounds) {
  std::deque<QPointF> trimmed_polyline(polyline.begin(), polyline.end());
  maybeTrimFront(trimmed_polyline, bounds.first);
  maybeTrimBack(trimmed_polyline, bounds.second);

  return std::vector<QPointF>(trimmed_polyline.begin(), trimmed_polyline.end());
}

bool DistortionModelBuilder::maybeTrimFront(std::deque<QPointF>& polyline, const QLineF& bound) {
  if (sidesOfLine(bound, polyline.front(), polyline.back()) >= 0) {
    // Doesn't need trimming.
    return false;
  }

  while (polyline.size() > 2 && sidesOfLine(bound, polyline.front(), polyline[1]) > 0) {
    polyline.pop_front();
  }

  intersectFront(polyline, bound);

  return true;
}

bool DistortionModelBuilder::maybeTrimBack(std::deque<QPointF>& polyline, const QLineF& bound) {
  if (sidesOfLine(bound, polyline.front(), polyline.back()) >= 0) {
    // Doesn't need trimming.
    return false;
  }

  while (polyline.size() > 2 && sidesOfLine(bound, polyline[polyline.size() - 2], polyline.back()) > 0) {
    polyline.pop_back();
  }

  intersectBack(polyline, bound);

  return true;
}

void DistortionModelBuilder::intersectFront(std::deque<QPointF>& polyline, const QLineF& bound) {
  assert(polyline.size() >= 2);

  const QLineF front_segment(polyline.front(), polyline[1]);
  QPointF intersection;
  if (bound.intersect(front_segment, &intersection) != QLineF::NoIntersection) {
    polyline.front() = intersection;
  }
}

void DistortionModelBuilder::intersectBack(std::deque<QPointF>& polyline, const QLineF& bound) {
  assert(polyline.size() >= 2);

  const QLineF back_segment(polyline[polyline.size() - 2], polyline.back());
  QPointF intersection;
  if (bound.intersect(back_segment, &intersection) != QLineF::NoIntersection) {
    polyline.back() = intersection;
  }
}

XSpline DistortionModelBuilder::fitExtendedSpline(const std::vector<QPointF>& polyline,
                                                  const Vec2d& centroid,
                                                  const std::pair<QLineF, QLineF>& bounds) {
  using namespace spfit;

  const QLineF chord(polyline.front(), polyline.back());
  XSpline spline;
  const int initial_spline_points = 5;
  spline.appendControlPoint(chord.pointAt(0), 1);
  for (int i = 1; i < initial_spline_points - 1; ++i) {
    const double fraction = i / (initial_spline_points - 1.0);
    spline.appendControlPoint(chord.pointAt(fraction), 1);
  }
  spline.appendControlPoint(chord.pointAt(1), 1);

  // initialSplinePositioning(spline, 0, chord.p1(), 1, chord.p2());

  class ModelShape : public PolylineModelShape {
   public:
    explicit ModelShape(const std::vector<QPointF>& polyline) : PolylineModelShape(polyline) {}

   protected:
    SqDistApproximant calcApproximant(const QPointF& pt,
                                      FittableSpline::SampleFlags sample_flags,
                                      Flags polyline_flags,
                                      const FrenetFrame& frenet_frame,
                                      double signed_curvature) const override {
      if (polyline_flags & (POLYLINE_FRONT | POLYLINE_BACK)) {
        if (sample_flags & FittableSpline::JUNCTION_SAMPLE) {
          return SqDistApproximant::pointDistance(frenet_frame.origin());
        } else {
          return SqDistApproximant();
        }
      } else {
        return SqDistApproximant::curveDistance(pt, frenet_frame, signed_curvature);
      }
    }
  };


  const ModelShape model_shape(polyline);
  SplineFitter fitter(&spline);

  FittableSpline::SamplingParams sampling_params;
  sampling_params.maxDistBetweenSamples = 10;
  fitter.setSamplingParams(sampling_params);

  int iterations_remaining = 20;
  LinearForceBalancer balancer(0.8);
  balancer.setTargetRatio(0.1);
  balancer.setIterationsToTarget(iterations_remaining - 1);

  // Initial fitting: just uniform distribution of junction points on a spline.
  {
    ConstraintSet constraints(&spline);
    constraints.constrainSplinePoint(0, bounds.first);
    constraints.constrainSplinePoint(1, bounds.second);
    for (int i = 0; i < initial_spline_points; ++i) {
      constraints.constrainSplinePoint(spline.controlPointIndexToT(i), chord);
    }
    fitter.setConstraints(constraints);
    fitter.addInternalForce(spline.junctionPointsAttractionForce());

    // We don't have any external forces, so we can choose any non-zero
    // weight for internal force.
    fitter.optimize(1);
    assert(!Curve::splineHasLoops(spline));
  }

  ConstraintSet constraints(&spline);
  constraints.constrainSplinePoint(0, bounds.first);
  constraints.constrainSplinePoint(1, bounds.second);
  fitter.setConstraints(constraints);

  for (int iteration = 0; iterations_remaining > 0; ++iteration, --iterations_remaining, balancer.nextIteration()) {
    fitter.addAttractionForces(model_shape);
    fitter.addInternalForce(spline.controlPointsAttractionForce());

    double internal_force_weight = balancer.calcInternalForceWeight(fitter.internalForce(), fitter.externalForce());
    const OptimizationResult res(fitter.optimize(internal_force_weight));
    if (Curve::splineHasLoops(spline)) {
      if (iteration == 0) {
        // Having a loop on the first iteration is not good at all.
        throw BadCurve();
      } else {
        fitter.undoLastStep();
        break;
      }
    }

    if (res.improvementPercentage() < 0.5) {
      break;
    }
  }

  return spline;
}  // DistortionModelBuilder::fitExtendedSpline

/*============================== RansacAlgo ============================*/

void DistortionModelBuilder::RansacAlgo::buildAndAssessModel(const TracedCurve* top_curve,
                                                             const TracedCurve* bottom_curve) try {
  DistortionModel model;
  model.setTopCurve(Curve(top_curve->extendedPolyline));
  model.setBottomCurve(Curve(bottom_curve->extendedPolyline));
  if (!model.isValid()) {
    return;
  }

  const double depth_perception = 2.0;  // Doesn't matter much here.
  const CylindricalSurfaceDewarper dewarper(top_curve->extendedPolyline, bottom_curve->extendedPolyline,
                                            depth_perception);

  double error = 0;
  for (const TracedCurve& curve : m_rAllCurves) {
    const size_t polyline_size = curve.trimmedPolyline.size();
    const double r_reference_height = 1.0 / 1.0;  // calcReferenceHeight(dewarper, curve.centroid);

    // We are going to approximate the dewarped polyline by a straight line
    // using linear least-squares: At*A*x = At*B -> x = (At*A)-1 * At*B
    std::vector<double> At;
    At.reserve(polyline_size * 2);
    std::vector<double> B;
    B.reserve(polyline_size);

    for (const QPointF& warped_pt : curve.trimmedPolyline) {
      // TODO: add another signature with hint for efficiency.
      const QPointF dewarped_pt(dewarper.mapToDewarpedSpace(warped_pt));

      // ax + b = y  <-> x * a + 1 * b = y
      At.push_back(dewarped_pt.x());
      At.push_back(1);
      B.push_back(dewarped_pt.y());
    }

    DynamicMatrixCalc<double> mc;

    // A = Att
    boost::scoped_array<double> A(new double[polyline_size * 2]);
    mc(&At[0], 2, (int) polyline_size).transWrite(&A[0]);

    try {
      boost::scoped_array<double> errvec(new double[polyline_size]);
      double ab[2];  // As in "y = ax + b".

      // errvec = B - A * (At*A)-1 * At * B
      // ab = (At*A)-1 * At * B
      (mc(&B[0], (int) polyline_size, 1)
       - mc(&A[0], (int) polyline_size, 2)
             * ((mc(&At[0], 2, (int) polyline_size) * mc(&A[0], (int) polyline_size, 2)).inv()
                * (mc(&At[0], 2, (int) polyline_size) * mc(&B[0], (int) polyline_size, 1)))
                   .write(ab))
          .write(&errvec[0]);

      double sum_abs_err = 0;
      for (size_t i = 0; i < polyline_size; ++i) {
        sum_abs_err += std::fabs(errvec[i]) * r_reference_height;
      }
      // Penalty for not being straight.
      error += sum_abs_err / polyline_size;

      // TODO: penalty for not being horizontal.
    } catch (const std::runtime_error&) {
      // Strictly vertical line?
      error += 1000;
    }
  }

  if (error < m_bestModel.totalError) {
    m_bestModel.topCurve = top_curve;
    m_bestModel.bottomCurve = bottom_curve;
    m_bestModel.totalError = error;
  }
}  // DistortionModelBuilder::RansacAlgo::buildAndAssessModel
catch (const std::runtime_error&) {
  // Probably CylindricalSurfaceDewarper didn't like something.
}

#if 0
    double DistortionModelBuilder::RansacAlgo::calcReferenceHeight(const CylindricalSurfaceDewarper& dewarper,
                                                                   const QPointF& loc) {
    // TODO: ideally, we would use the counterpart of CylindricalSurfaceDewarper::mapGeneratrix(),
    // that would map it the other way, and which doesn't currently exist.

        const QPointF pt1(dewarper.mapToDewarpedSpace(loc + QPointF(0.0, -10)));
        const QPointF pt2(dewarper.mapToDewarpedSpace(loc + QPointF(0.0, 10)));

        return std::fabs(pt1.y() - pt2.y());
    }
#endif

QImage DistortionModelBuilder::visualizeTrimmedPolylines(const QImage& background,
                                                         const std::vector<TracedCurve>& curves) const {
  QImage canvas(background.convertToFormat(QImage::Format_RGB32));
  QPainter painter(&canvas);
  painter.setRenderHint(QPainter::Antialiasing);

  const int width = background.width();
  const int height = background.height();
  const double stroke_width = std::sqrt(double(width * width + height * height)) / 500;

  // Extend / trim bounds.
  QLineF bound1(m_bound1);
  QLineF bound2(m_bound2);
  lineBoundedByRect(bound1, background.rect());
  lineBoundedByRect(bound2, background.rect());

  // Draw bounds.
  QPen pen(QColor(0, 0, 255, 180));
  pen.setWidthF(stroke_width);
  painter.setPen(pen);
  painter.drawLine(bound1);
  painter.drawLine(bound2);

  for (const TracedCurve& curve : curves) {
    if (!curve.trimmedPolyline.empty()) {
      painter.drawPolyline(&curve.trimmedPolyline[0], static_cast<int>(curve.trimmedPolyline.size()));
    }
  }

  // Draw polyline knots.
  QBrush knot_brush(Qt::magenta);
  painter.setBrush(knot_brush);
  painter.setPen(Qt::NoPen);
  for (const TracedCurve& curve : curves) {
    QRectF rect(0, 0, stroke_width, stroke_width);
    for (const QPointF& knot : curve.trimmedPolyline) {
      rect.moveCenter(knot);
      painter.drawEllipse(rect);
    }
  }

  return canvas;
}  // DistortionModelBuilder::visualizeTrimmedPolylines

QImage DistortionModelBuilder::visualizeModel(const QImage& background,
                                              const std::vector<TracedCurve>& curves,
                                              const RansacModel& model) const {
  QImage canvas(background.convertToFormat(QImage::Format_RGB32));
  QPainter painter(&canvas);
  painter.setRenderHint(QPainter::Antialiasing);

  const int width = background.width();
  const int height = background.height();
  const double stroke_width = std::sqrt(double(width * width + height * height)) / 500;

  // Extend / trim bounds.
  QLineF bound1(m_bound1);
  QLineF bound2(m_bound2);
  lineBoundedByRect(bound1, background.rect());
  lineBoundedByRect(bound2, background.rect());
  // Draw bounds.
  QPen bounds_pen(QColor(0, 0, 255, 180));
  bounds_pen.setWidthF(stroke_width);
  painter.setPen(bounds_pen);
  painter.drawLine(bound1);
  painter.drawLine(bound2);

  QPen active_curve_pen(QColor(0x45, 0xff, 0x53, 180));
  active_curve_pen.setWidthF(stroke_width);

  QPen inactive_curve_pen(QColor(0, 0, 255, 140));
  inactive_curve_pen.setWidthF(stroke_width);

  QPen reverse_segments_pen(QColor(0xff, 0x28, 0x05, 140));
  reverse_segments_pen.setWidthF(stroke_width);

  QBrush control_point_brush(QColor(0xff, 0x00, 0x00, 255));

  QBrush junction_point_brush(QColor(0xff, 0x00, 0xff, 255));

  for (const TracedCurve& curve : curves) {
    if (curve.extendedPolyline.empty()) {
      continue;
    }
    if ((&curve == model.topCurve) || (&curve == model.bottomCurve)) {
      painter.setPen(active_curve_pen);
    } else {
      painter.setPen(inactive_curve_pen);
    }

    const size_t size = curve.extendedPolyline.size();
    painter.drawPolyline(&curve.extendedPolyline[0], static_cast<int>(curve.extendedPolyline.size()));

    const Vec2d main_direction(curve.extendedPolyline.back() - curve.extendedPolyline.front());
    std::list<std::vector<int>> reverse_segments;

    for (size_t i = 1; i < size; ++i) {
      const Vec2d dir(curve.extendedPolyline[i] - curve.extendedPolyline[i - 1]);
      if (dir.dot(main_direction) >= 0) {
        continue;
      }

      // We've got a reverse segment.
      if (!reverse_segments.empty() && (reverse_segments.back().back() == int(i) - 1)) {
        // Continue the previous sequence.
        reverse_segments.back().push_back(static_cast<int&&>(i));
      } else {
        // Start a new sequence.
        reverse_segments.emplace_back();
        std::vector<int>& sequence = reverse_segments.back();
        sequence.push_back(static_cast<int&&>(i - 1));
        sequence.push_back(static_cast<int&&>(i));
      }
    }

    QVector<QPointF> polyline;

    if (!reverse_segments.empty()) {
      painter.setPen(reverse_segments_pen);
      for (const std::vector<int>& sequence : reverse_segments) {
        assert(!sequence.empty());
        polyline.clear();
        for (int idx : sequence) {
          polyline << curve.extendedPolyline[idx];
        }
        painter.drawPolyline(polyline);
      }
    }

    const int num_control_points = curve.extendedSpline.numControlPoints();
    QRectF rect(0, 0, stroke_width, stroke_width);

    // Draw junction points.
    painter.setPen(Qt::NoPen);
    painter.setBrush(junction_point_brush);
    for (int i = 0; i < num_control_points; ++i) {
      const double t = curve.extendedSpline.controlPointIndexToT(i);
      rect.moveCenter(curve.extendedSpline.pointAt(t));
      painter.drawEllipse(rect);
    }
    // Draw control points.
    painter.setPen(Qt::NoPen);
    painter.setBrush(control_point_brush);
    for (int i = 0; i < num_control_points; ++i) {
      rect.moveCenter(curve.extendedSpline.controlPointPosition(i));
      painter.drawEllipse(rect);
    }
  }

  return canvas;
}  // DistortionModelBuilder::visualizeModel
}  // namespace dewarping