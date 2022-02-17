// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "DistortionModelBuilder.h"

#include <QDebug>
#include <QImage>
#include <QPainter>
#if QT_VERSION_MAJOR > 5 or QT_VERSION_MINOR > 9
#include <QRandomGenerator>
#endif
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

  TracedCurve(const std::vector<QPointF>& trimmedPolyline, const XSpline& extendedSpline, double ord)
      : trimmedPolyline(trimmedPolyline),
        extendedPolyline(extendedSpline.toPolyline()),
        extendedSpline(extendedSpline),
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
  explicit RansacAlgo(const std::vector<TracedCurve>& allCurves) : m_allCurves(allCurves) {}

  void buildAndAssessModel(const TracedCurve* topCurve, const TracedCurve* bottomCurve);

  RansacModel& bestModel() { return m_bestModel; }

  const RansacModel& bestModel() const { return m_bestModel; }

 private:
  double calcReferenceHeight(const CylindricalSurfaceDewarper& dewarper, const QPointF& loc);

  RansacModel m_bestModel;
  const std::vector<TracedCurve>& m_allCurves;
};


class DistortionModelBuilder::BadCurve : public std::exception {
 public:
  const char* what() const noexcept override { return "Bad curve"; }
};


DistortionModelBuilder::DistortionModelBuilder(const Vec2d& downDirection)
    : m_downDirection(downDirection), m_rightDirection(downDirection[1], -downDirection[0]) {
  assert(downDirection.squaredNorm() > 0);
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

  const QLineF downLine(xform.map(QLineF(QPointF(0, 0), m_downDirection)));
  const QLineF rightLine(xform.map(QLineF(QPointF(0, 0), m_rightDirection)));

  m_downDirection = downLine.p2() - downLine.p1();
  m_rightDirection = rightLine.p2() - rightLine.p1();
  m_bound1 = xform.map(m_bound1);
  m_bound2 = xform.map(m_bound2);

  for (std::vector<QPointF>& polyline : m_ltrPolylines) {
    for (QPointF& pt : polyline) {
      pt = xform.map(pt);
    }
  }
}

DistortionModel DistortionModelBuilder::tryBuildModel(DebugImages* dbg, const QImage* dbgBackground) const {
  auto numCurves = static_cast<int>(m_ltrPolylines.size());

  if ((numCurves < 2) || (m_bound1.p1() == m_bound1.p2()) || (m_bound2.p1() == m_bound2.p2())) {
    return DistortionModel();
  }

  std::vector<TracedCurve> orderedCurves;
  orderedCurves.reserve(numCurves);

  for (const std::vector<QPointF>& polyline : m_ltrPolylines) {
    try {
      orderedCurves.push_back(polylineToCurve(polyline));
    } catch (const BadCurve&) {
      // Just skip it.
    }
  }
  numCurves = static_cast<int>(orderedCurves.size());
  if (numCurves == 0) {
    return DistortionModel();
  }
  // if (numCurves < 2) {
  // return DistortionModel();
  // }
  std::sort(orderedCurves.begin(), orderedCurves.end());

  // Select the best pair using RANSAC.
  RansacAlgo ransac(orderedCurves);

  // First let's try to combine each of the 3 top-most lines
  // with each of the 3 bottom-most ones.
  for (int i = 0; i < std::min<int>(3, numCurves); ++i) {
    for (int j = std::max<int>(0, numCurves - 3); j < numCurves; ++j) {
      if (i < j) {
        ransac.buildAndAssessModel(&orderedCurves[i], &orderedCurves[j]);
      }
    }
  }
  // Continue by throwing in some random pairs of lines.
#if QT_VERSION_MAJOR == 5 and QT_VERSION_MINOR <= 9
  qsrand(0); // Repeatablity is important.
#else
  QRandomGenerator prng(0); // Repeatablity is important.
#endif
  int randomPairsRemaining = 10;
  while (randomPairsRemaining-- > 0) {
#if QT_VERSION_MAJOR == 5 and QT_VERSION_MINOR <= 9
    int i = qrand() % numCurves;
    int j = qrand() % numCurves;
#else
    int i = prng.generate() % numCurves;
    int j = prng.generate() % numCurves;
#endif
    if (i > j) {
      std::swap(i, j);
    }
    if (i < j) {
      ransac.buildAndAssessModel(&orderedCurves[i], &orderedCurves[j]);
    }
  }

  if (dbg && dbgBackground) {
    dbg->add(visualizeTrimmedPolylines(*dbgBackground, orderedCurves), "trimmed_polylines");
    dbg->add(visualizeModel(*dbgBackground, orderedCurves, ransac.bestModel()), "distortion_model");
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
  const std::vector<QPointF> trimmedPolyline(maybeTrimPolyline(polyline, bounds));

  const Vec2d centroid(this->centroid(polyline));

  // Fit the polyline to a spline, extending it to bounds at the same time.
  const XSpline extendedSpline(fitExtendedSpline(trimmedPolyline, centroid, bounds));

  const double order = centroid.dot(m_downDirection);
  return TracedCurve(trimmedPolyline, extendedSpline, order);
}

Vec2d DistortionModelBuilder::centroid(const std::vector<QPointF>& polyline) {
  const auto numPoints = static_cast<int>(polyline.size());
  if (numPoints == 0) {
    return Vec2d();
  } else if (numPoints == 1) {
    return Vec2d(polyline.front());
  }

  Vec2d accum(0, 0);
  double totalWeight = 0;

  for (int i = 1; i < numPoints; ++i) {
    const QLineF segment(polyline[i - 1], polyline[i]);
    const Vec2d center(0.5 * (segment.p1() + segment.p2()));
    const double weight = segment.length();
    accum += center * weight;
    totalWeight += weight;
  }

  if (totalWeight < 1e-06) {
    return Vec2d(polyline.front());
  } else {
    return accum / totalWeight;
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
  std::deque<QPointF> trimmedPolyline(polyline.begin(), polyline.end());
  maybeTrimFront(trimmedPolyline, bounds.first);
  maybeTrimBack(trimmedPolyline, bounds.second);
  return std::vector<QPointF>(trimmedPolyline.begin(), trimmedPolyline.end());
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

  const QLineF frontSegment(polyline.front(), polyline[1]);
  QPointF intersection;
#if QT_VERSION_MAJOR == 5 and QT_VERSION_MINOR < 14
  auto intersect = bound.intersect(frontSegment, &intersection) ;
#else
  auto intersect = bound.intersects(frontSegment, &intersection) ;
#endif
  if (intersect != QLineF::NoIntersection) {
    polyline.front() = intersection;
  }
}

void DistortionModelBuilder::intersectBack(std::deque<QPointF>& polyline, const QLineF& bound) {
  assert(polyline.size() >= 2);

  const QLineF backSegment(polyline[polyline.size() - 2], polyline.back());
  QPointF intersection;
#if QT_VERSION_MAJOR == 5 and QT_VERSION_MINOR < 14
  auto intersect = bound.intersect(backSegment, &intersection) ;
#else
  auto intersect = bound.intersects(backSegment, &intersection) ;
#endif
  if (intersect != QLineF::NoIntersection) {
    polyline.back() = intersection;
  }
}

XSpline DistortionModelBuilder::fitExtendedSpline(const std::vector<QPointF>& polyline,
                                                  const Vec2d& centroid,
                                                  const std::pair<QLineF, QLineF>& bounds) {
  using namespace spfit;

  const QLineF chord(polyline.front(), polyline.back());
  XSpline spline;
  const int initialSplinePoints = 5;
  spline.appendControlPoint(chord.pointAt(0), 1);
  for (int i = 1; i < initialSplinePoints - 1; ++i) {
    const double fraction = i / (initialSplinePoints - 1.0);
    spline.appendControlPoint(chord.pointAt(fraction), 1);
  }
  spline.appendControlPoint(chord.pointAt(1), 1);

  // initialSplinePositioning(spline, 0, chord.p1(), 1, chord.p2());

  class ModelShape : public PolylineModelShape {
   public:
    explicit ModelShape(const std::vector<QPointF>& polyline) : PolylineModelShape(polyline) {}

   protected:
    SqDistApproximant calcApproximant(const QPointF& pt,
                                      FittableSpline::SampleFlags sampleFlags,
                                      Flags polylineFlags,
                                      const FrenetFrame& frenetFrame,
                                      double signedCurvature) const override {
      if (polylineFlags & (POLYLINE_FRONT | POLYLINE_BACK)) {
        if (sampleFlags & FittableSpline::JUNCTION_SAMPLE) {
          return SqDistApproximant::pointDistance(frenetFrame.origin());
        } else {
          return SqDistApproximant();
        }
      } else {
        return SqDistApproximant::curveDistance(pt, frenetFrame, signedCurvature);
      }
    }
  };


  const ModelShape modelShape(polyline);
  SplineFitter fitter(&spline);

  FittableSpline::SamplingParams samplingParams;
  samplingParams.maxDistBetweenSamples = 10;
  fitter.setSamplingParams(samplingParams);

  int iterationsRemaining = 20;
  LinearForceBalancer balancer(0.8);
  balancer.setTargetRatio(0.1);
  balancer.setIterationsToTarget(iterationsRemaining - 1);

  // Initial fitting: just uniform distribution of junction points on a spline.
  {
    ConstraintSet constraints(&spline);
    constraints.constrainSplinePoint(0, bounds.first);
    constraints.constrainSplinePoint(1, bounds.second);
    for (int i = 0; i < initialSplinePoints; ++i) {
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

  for (int iteration = 0; iterationsRemaining > 0; ++iteration, --iterationsRemaining, balancer.nextIteration()) {
    fitter.addAttractionForces(modelShape);
    fitter.addInternalForce(spline.controlPointsAttractionForce());

    double internalForceWeight = balancer.calcInternalForceWeight(fitter.internalForce(), fitter.externalForce());
    const OptimizationResult res(fitter.optimize(internalForceWeight));
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

void DistortionModelBuilder::RansacAlgo::buildAndAssessModel(const TracedCurve* topCurve,
                                                             const TracedCurve* bottomCurve) try {
  DistortionModel model;
  model.setTopCurve(Curve(topCurve->extendedPolyline));
  model.setBottomCurve(Curve(bottomCurve->extendedPolyline));
  if (!model.isValid()) {
    return;
  }

  const double depthPerception = 2.0;  // Doesn't matter much here.
  const CylindricalSurfaceDewarper dewarper(topCurve->extendedPolyline, bottomCurve->extendedPolyline, depthPerception);

  double error = 0;
  for (const TracedCurve& curve : m_allCurves) {
    const size_t polylineSize = curve.trimmedPolyline.size();
    const double rReferenceHeight = 1.0 / 1.0;  // calcReferenceHeight(dewarper, curve.centroid);

    // We are going to approximate the dewarped polyline by a straight line
    // using linear least-squares: At*A*x = At*B -> x = (At*A)-1 * At*B
    std::vector<double> At;
    At.reserve(polylineSize * 2);
    std::vector<double> B;
    B.reserve(polylineSize);

    for (const QPointF& warpedPt : curve.trimmedPolyline) {
      // TODO: add another signature with hint for efficiency.
      const QPointF dewarpedPt(dewarper.mapToDewarpedSpace(warpedPt));

      // ax + b = y  <-> x * a + 1 * b = y
      At.push_back(dewarpedPt.x());
      At.push_back(1);
      B.push_back(dewarpedPt.y());
    }

    DynamicMatrixCalc<double> mc;

    // A = Att
    boost::scoped_array<double> A(new double[polylineSize * 2]);
    mc(&At[0], 2, (int) polylineSize).transWrite(&A[0]);

    try {
      boost::scoped_array<double> errvec(new double[polylineSize]);
      double ab[2];  // As in "y = ax + b".

      // errvec = B - A * (At*A)-1 * At * B
      // ab = (At*A)-1 * At * B
      (mc(&B[0], (int) polylineSize, 1)
       - mc(&A[0], (int) polylineSize, 2)
             * ((mc(&At[0], 2, (int) polylineSize) * mc(&A[0], (int) polylineSize, 2)).inv()
                * (mc(&At[0], 2, (int) polylineSize) * mc(&B[0], (int) polylineSize, 1)))
                   .write(ab))
          .write(&errvec[0]);

      double sumAbsErr = 0;
      for (size_t i = 0; i < polylineSize; ++i) {
        sumAbsErr += std::fabs(errvec[i]) * rReferenceHeight;
      }
      // Penalty for not being straight.
      error += sumAbsErr / polylineSize;

      // TODO: penalty for not being horizontal.
    } catch (const std::runtime_error&) {
      // Strictly vertical line?
      error += 1000;
    }
  }

  if (error < m_bestModel.totalError) {
    m_bestModel.topCurve = topCurve;
    m_bestModel.bottomCurve = bottomCurve;
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
  const double strokeWidth = std::sqrt(double(width * width + height * height)) / 500;

  // Extend / trim bounds.
  QLineF bound1(m_bound1);
  QLineF bound2(m_bound2);
  lineBoundedByRect(bound1, background.rect());
  lineBoundedByRect(bound2, background.rect());

  // Draw bounds.
  QPen pen(QColor(0, 0, 255, 180));
  pen.setWidthF(strokeWidth);
  painter.setPen(pen);
  painter.drawLine(bound1);
  painter.drawLine(bound2);

  for (const TracedCurve& curve : curves) {
    if (!curve.trimmedPolyline.empty()) {
      painter.drawPolyline(&curve.trimmedPolyline[0], static_cast<int>(curve.trimmedPolyline.size()));
    }
  }

  // Draw polyline knots.
  QBrush knotBrush(Qt::magenta);
  painter.setBrush(knotBrush);
  painter.setPen(Qt::NoPen);
  for (const TracedCurve& curve : curves) {
    QRectF rect(0, 0, strokeWidth, strokeWidth);
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
  const double strokeWidth = std::sqrt(double(width * width + height * height)) / 500;

  // Extend / trim bounds.
  QLineF bound1(m_bound1);
  QLineF bound2(m_bound2);
  lineBoundedByRect(bound1, background.rect());
  lineBoundedByRect(bound2, background.rect());
  // Draw bounds.
  QPen boundsPen(QColor(0, 0, 255, 180));
  boundsPen.setWidthF(strokeWidth);
  painter.setPen(boundsPen);
  painter.drawLine(bound1);
  painter.drawLine(bound2);

  QPen activeCurvePen(QColor(0x45, 0xff, 0x53, 180));
  activeCurvePen.setWidthF(strokeWidth);

  QPen inactiveCurvePen(QColor(0, 0, 255, 140));
  inactiveCurvePen.setWidthF(strokeWidth);

  QPen reverseSegmentsPen(QColor(0xff, 0x28, 0x05, 140));
  reverseSegmentsPen.setWidthF(strokeWidth);

  QBrush controlPointBrush(QColor(0xff, 0x00, 0x00, 255));

  QBrush junctionPointBrush(QColor(0xff, 0x00, 0xff, 255));

  for (const TracedCurve& curve : curves) {
    if (curve.extendedPolyline.empty()) {
      continue;
    }
    if ((&curve == model.topCurve) || (&curve == model.bottomCurve)) {
      painter.setPen(activeCurvePen);
    } else {
      painter.setPen(inactiveCurvePen);
    }

    const size_t size = curve.extendedPolyline.size();
    painter.drawPolyline(&curve.extendedPolyline[0], static_cast<int>(curve.extendedPolyline.size()));

    const Vec2d mainDirection(curve.extendedPolyline.back() - curve.extendedPolyline.front());
    std::list<std::vector<int>> reverseSegments;

    for (size_t i = 1; i < size; ++i) {
      const Vec2d dir(curve.extendedPolyline[i] - curve.extendedPolyline[i - 1]);
      if (dir.dot(mainDirection) >= 0) {
        continue;
      }

      // We've got a reverse segment.
      if (!reverseSegments.empty() && (reverseSegments.back().back() == int(i) - 1)) {
        // Continue the previous sequence.
        reverseSegments.back().push_back(static_cast<int&&>(i));
      } else {
        // Start a new sequence.
        reverseSegments.emplace_back();
        std::vector<int>& sequence = reverseSegments.back();
        sequence.push_back(static_cast<int&&>(i - 1));
        sequence.push_back(static_cast<int&&>(i));
      }
    }

    QVector<QPointF> polyline;

    if (!reverseSegments.empty()) {
      painter.setPen(reverseSegmentsPen);
      for (const std::vector<int>& sequence : reverseSegments) {
        assert(!sequence.empty());
        polyline.clear();
        for (int idx : sequence) {
          polyline << curve.extendedPolyline[idx];
        }
        painter.drawPolyline(polyline);
      }
    }

    const int numControlPoints = curve.extendedSpline.numControlPoints();
    QRectF rect(0, 0, strokeWidth, strokeWidth);

    // Draw junction points.
    painter.setPen(Qt::NoPen);
    painter.setBrush(junctionPointBrush);
    for (int i = 0; i < numControlPoints; ++i) {
      const double t = curve.extendedSpline.controlPointIndexToT(i);
      rect.moveCenter(curve.extendedSpline.pointAt(t));
      painter.drawEllipse(rect);
    }
    // Draw control points.
    painter.setPen(Qt::NoPen);
    painter.setBrush(controlPointBrush);
    for (int i = 0; i < numControlPoints; ++i) {
      rect.moveCenter(curve.extendedSpline.controlPointPosition(i));
      painter.drawEllipse(rect);
    }
  }
  return canvas;
}  // DistortionModelBuilder::visualizeModel
}  // namespace dewarping
