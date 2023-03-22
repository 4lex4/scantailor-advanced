// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "DewarpingView.h"

#include <CylindricalSurfaceDewarper.h>

#include <QDebug>
#include <QPainter>
#include <QShortcut>
#include <boost/bind/bind.hpp>

#include "ImagePresentation.h"
#include "ToLineProjector.h"
#include "spfit/ConstraintSet.h"
#include "spfit/LinearForceBalancer.h"
#include "spfit/PolylineModelShape.h"
#include "spfit/SplineFitter.h"

namespace output {
DewarpingView::DewarpingView(const QImage& image,
                             const ImagePixmapUnion& downscaledImage,
                             const QTransform& imageToVirt,
                             const QPolygonF& virtDisplayArea,
                             const QRectF& virtContentRect,
                             const PageId& pageId,
                             DewarpingOptions dewarpingOptions,
                             const dewarping::DistortionModel& distortionModel,
                             const DepthPerception& depthPerception)
    : ImageViewBase(image, downscaledImage, ImagePresentation(imageToVirt, virtDisplayArea)),
      m_pageId(pageId),
      m_virtDisplayArea(virtDisplayArea),
      m_dewarpingOptions(dewarpingOptions),
      m_distortionModel(distortionModel),
      m_depthPerception(depthPerception),
      m_topSpline(*this),
      m_bottomSpline(*this),
      m_dragHandler(*this),
      m_zoomHandler(*this) {
  setMouseTracking(true);

  const QPolygonF sourceContentRect(virtualToImage().map(virtContentRect));

  XSpline topSpline(m_distortionModel.topCurve().xspline());
  XSpline bottomSpline(m_distortionModel.bottomCurve().xspline());
  if (topSpline.numControlPoints() < 2) {
    const std::vector<QPointF>& polyline = m_distortionModel.topCurve().polyline();

    XSpline newTopSpline;
    if (polyline.size() < 2) {
      initNewSpline(newTopSpline, sourceContentRect[0], sourceContentRect[1], &dewarpingOptions);
    } else {
      initNewSpline(newTopSpline, polyline.front(), polyline.back(), &dewarpingOptions);
      fitSpline(newTopSpline, polyline);
    }

    topSpline.swap(newTopSpline);
  }
  if (bottomSpline.numControlPoints() < 2) {
    const std::vector<QPointF>& polyline = m_distortionModel.bottomCurve().polyline();

    XSpline newBottomSpline;
    if (polyline.size() < 2) {
      initNewSpline(newBottomSpline, sourceContentRect[3], sourceContentRect[2], &dewarpingOptions);
    } else {
      initNewSpline(newBottomSpline, polyline.front(), polyline.back(), &dewarpingOptions);
      fitSpline(newBottomSpline, polyline);
    }

    bottomSpline.swap(newBottomSpline);
  }

  m_topSpline.setSpline(topSpline);
  m_bottomSpline.setSpline(bottomSpline);

  InteractiveXSpline* splines[2] = {&m_topSpline, &m_bottomSpline};
  int curveIdx = -1;
  for (InteractiveXSpline* spline : splines) {
    ++curveIdx;
    spline->setModifiedCallback(boost::bind(&DewarpingView::curveModified, this, curveIdx));
    spline->setDragFinishedCallback(boost::bind(&DewarpingView::dragFinished, this));
    spline->setStorageTransform(boost::bind(&DewarpingView::sourceToWidget, this, boost::placeholders::_1),
                                boost::bind(&DewarpingView::widgetToSource, this, boost::placeholders::_1));
    makeLastFollower(*spline);
  }

  m_distortionModel.setTopCurve(dewarping::Curve(m_topSpline.spline()));
  m_distortionModel.setBottomCurve(dewarping::Curve(m_bottomSpline.spline()));

  rootInteractionHandler().makeLastFollower(*this);
  rootInteractionHandler().makeLastFollower(m_dragHandler);
  rootInteractionHandler().makeLastFollower(m_zoomHandler);

  m_removeControlPointShortcut = new QShortcut(Qt::Key_D, this);
  QObject::connect(m_removeControlPointShortcut, &QShortcut::activated, [this]() {
    m_topSpline.removeControlPointUnderMouse();
    m_bottomSpline.removeControlPointUnderMouse();
  });
}

DewarpingView::~DewarpingView() = default;

void DewarpingView::initNewSpline(XSpline& spline,
                                  const QPointF& p1,
                                  const QPointF& p2,
                                  const DewarpingOptions* dewarpingOptions) {
  const QLineF line(p1, p2);
  spline.appendControlPoint(line.p1(), 0);
  if ((*dewarpingOptions).dewarpingMode() == AUTO) {
    spline.appendControlPoint(line.pointAt(1.0 / 4.0), 1);
    spline.appendControlPoint(line.pointAt(2.0 / 4.0), 1);
    spline.appendControlPoint(line.pointAt(3.0 / 4.0), 1);
  }
  spline.appendControlPoint(line.p2(), 0);
}

void DewarpingView::fitSpline(XSpline& spline, const std::vector<QPointF>& polyline) {
  using namespace spfit;

  SplineFitter fitter(&spline);
  const PolylineModelShape modelShape(polyline);

  ConstraintSet constraints(&spline);
  constraints.constrainSplinePoint(0.0, polyline.front());
  constraints.constrainSplinePoint(1.0, polyline.back());
  fitter.setConstraints(constraints);

  FittableSpline::SamplingParams samplingParams;
  samplingParams.maxDistBetweenSamples = 10;
  fitter.setSamplingParams(samplingParams);

  int iterationsRemaining = 20;
  LinearForceBalancer balancer(0.8);
  balancer.setTargetRatio(0.1);
  balancer.setIterationsToTarget(iterationsRemaining - 1);

  for (; iterationsRemaining > 0; --iterationsRemaining, balancer.nextIteration()) {
    fitter.addAttractionForces(modelShape);
    fitter.addInternalForce(spline.controlPointsAttractionForce());

    double internalForceWeight = balancer.calcInternalForceWeight(fitter.internalForce(), fitter.externalForce());
    const OptimizationResult res(fitter.optimize(internalForceWeight));
    if (dewarping::Curve::splineHasLoops(spline)) {
      fitter.undoLastStep();
      break;
    }

    if (res.improvementPercentage() < 0.5) {
      break;
    }
  }
}  // DewarpingView::fitSpline

void DewarpingView::depthPerceptionChanged(double val) {
  m_depthPerception.setValue(val);
  update();
}

void DewarpingView::onPaint(QPainter& painter, const InteractionState& interaction) {
  painter.setRenderHint(QPainter::Antialiasing);

  painter.setPen(Qt::NoPen);
  painter.setBrush(QColor(0xff, 0xff, 0xff, 150));  // Translucent white.
  painter.drawPolygon(virtMarginArea(0));           // Left margin.
  painter.drawPolygon(virtMarginArea(1));           // Right margin.
  painter.setWorldTransform(imageToVirtual() * painter.worldTransform());
  painter.setBrush(Qt::NoBrush);

  QPen gridPen;
  gridPen.setColor(Qt::blue);
  gridPen.setCosmetic(true);
  gridPen.setWidthF(1.2);

  painter.setPen(gridPen);
  painter.setBrush(Qt::NoBrush);

  const int numVertGridLines = 30;
  const int numHorGridLines = 30;

  bool validModel = m_distortionModel.isValid();

  if (validModel) {
    try {
      std::vector<QVector<QPointF>> curves(numHorGridLines);

      dewarping::CylindricalSurfaceDewarper dewarper(m_distortionModel.topCurve().polyline(),
                                                     m_distortionModel.bottomCurve().polyline(),
                                                     m_depthPerception.value());
      dewarping::CylindricalSurfaceDewarper::State state;

      for (int j = 0; j < numVertGridLines; ++j) {
        const double x = j / (numVertGridLines - 1.0);
        const dewarping::CylindricalSurfaceDewarper::Generatrix gtx(dewarper.mapGeneratrix(x, state));
        const QPointF gtxP0(gtx.imgLine.pointAt(gtx.pln2img(0)));
        const QPointF gtxP1(gtx.imgLine.pointAt(gtx.pln2img(1)));
        painter.drawLine(gtxP0, gtxP1);
        for (int i = 0; i < numHorGridLines; ++i) {
          const double y = i / (numHorGridLines - 1.0);
          curves[i].push_back(gtx.imgLine.pointAt(gtx.pln2img(y)));
        }
      }

      for (const QVector<QPointF>& curve : curves) {
        painter.drawPolyline(curve);
      }
    } catch (const std::runtime_error&) {
      // Still probably a bad model, even though DistortionModel::isValid() was true.
      validModel = false;
    }
  }  // validModel
  if (!validModel) {
    // Just draw the frame.
    const dewarping::Curve& topCurve = m_distortionModel.topCurve();
    const dewarping::Curve& bottomCurve = m_distortionModel.bottomCurve();
    painter.drawLine(topCurve.polyline().front(), bottomCurve.polyline().front());
    painter.drawLine(topCurve.polyline().back(), bottomCurve.polyline().back());
#if QT_VERSION_MAJOR == 5 and QT_VERSION_MINOR < 14
    painter.drawPolyline(QVector<QPointF>::fromStdVector(topCurve.polyline()));
    painter.drawPolyline(QVector<QPointF>::fromStdVector(bottomCurve.polyline()));
#else
    painter.drawPolyline(QVector<QPointF>(topCurve.polyline().begin(), topCurve.polyline().end()));
    painter.drawPolyline(QVector<QPointF>(bottomCurve.polyline().begin(), bottomCurve.polyline().end()));
#endif
  }

  paintXSpline(painter, interaction, m_topSpline);
  paintXSpline(painter, interaction, m_bottomSpline);
}  // DewarpingView::onPaint

void DewarpingView::paintXSpline(QPainter& painter,
                                 const InteractionState& interaction,
                                 const InteractiveXSpline& ispline) {
  const XSpline& spline = ispline.spline();

  painter.save();
  painter.setBrush(Qt::NoBrush);

#if 0  // No point in drawing the curve itself - we already draw the grid.
        painter.setWorldTransform(imageToVirtual() * virtualToWidget());

        QPen curvePen(Qt::blue);
        curvePen.setWidthF(1.5);
        curvePen.setCosmetic(true);
        painter.setPen(curvePen);

        const std::vector<QPointF> polyline(spline.toPolyline());
        painter.drawPolyline(&polyline[0], polyline.size());
#endif
  // Drawing cosmetic points in transformed coordinates seems unreliable,
  // so let's draw them in widget coordinates.
  painter.setWorldMatrixEnabled(false);

  QPen existingPointPen(Qt::red);
  existingPointPen.setWidthF(4.0);
  existingPointPen.setCosmetic(true);
  painter.setPen(existingPointPen);

  const int numControlPoints = spline.numControlPoints();
  for (int i = 0; i < numControlPoints; ++i) {
    painter.drawPoint(sourceToWidget(spline.controlPointPosition(i)));
  }

  QPointF pt;
  if (ispline.curveIsProximityLeader(interaction, &pt)) {
    QPen newPointPen(existingPointPen);
    newPointPen.setColor(QColor(0x00ffff));
    painter.setPen(newPointPen);
    painter.drawPoint(pt);
  }

  painter.restore();
}  // DewarpingView::paintXSpline

void DewarpingView::curveModified(int curveIdx) {
  if (curveIdx == 0) {
    m_distortionModel.setTopCurve(dewarping::Curve(m_topSpline.spline()));
  } else {
    m_distortionModel.setBottomCurve(dewarping::Curve(m_bottomSpline.spline()));
  }
  update();
}

void DewarpingView::dragFinished() {
  if ((m_dewarpingOptions.dewarpingMode() == AUTO) || (m_dewarpingOptions.dewarpingMode() == MARGINAL)) {
    m_dewarpingOptions.setDewarpingMode(MANUAL);
  }
  emit distortionModelChanged(m_distortionModel);
}

/** Source image coordinates to widget coordinates. */
QPointF DewarpingView::sourceToWidget(const QPointF& pt) const {
  return virtualToWidget().map(imageToVirtual().map(pt));
}

/** Widget coordinates to source image coordinates. */
QPointF DewarpingView::widgetToSource(const QPointF& pt) const {
  return virtualToImage().map(widgetToVirtual().map(pt));
}

QPolygonF DewarpingView::virtMarginArea(int marginIdx) const {
  const dewarping::Curve& topCurve = m_distortionModel.topCurve();
  const dewarping::Curve& bottomCurve = m_distortionModel.bottomCurve();

  QLineF vertBoundary;   // From top to bottom, that's important!
  if (marginIdx == 0) {  // Left margin.
    vertBoundary.setP1(topCurve.polyline().front());
    vertBoundary.setP2(bottomCurve.polyline().front());
  } else {  // Right margin.
    vertBoundary.setP1(topCurve.polyline().back());
    vertBoundary.setP2(bottomCurve.polyline().back());
  }

  vertBoundary = imageToVirtual().map(vertBoundary);

  QLineF normal;
  if (marginIdx == 0) {  // Left margin.
    normal = QLineF(vertBoundary.p2(), vertBoundary.p1()).normalVector();
  } else {  // Right margin.
    normal = vertBoundary.normalVector();
  }

  // Project every vertex in the m_virtDisplayArea polygon
  // to vert_line and to its normal, keeping track min and max values.
  double min = NumericTraits<double>::max();
  double max = NumericTraits<double>::min();
  double normalMax = max;
  const ToLineProjector vertLineProjector(vertBoundary);
  const ToLineProjector normalProjector(normal);
  for (const QPointF& pt : m_virtDisplayArea) {
    const double p1 = vertLineProjector.projectionScalar(pt);
    if (p1 < min) {
      min = p1;
    }
    if (p1 > max) {
      max = p1;
    }

    const double p2 = normalProjector.projectionScalar(pt);
    if (p2 > normalMax) {
      normalMax = p2;
    }
  }

  // Workaround clipping bugs in QPolygon::intersected().
  min -= 1.0;
  max += 1.0;
  normalMax += 1.0;

  QPolygonF poly;
  poly << vertBoundary.pointAt(min);
  poly << vertBoundary.pointAt(max);
  poly << vertBoundary.pointAt(max) + normal.pointAt(normalMax) - normal.p1();
  poly << vertBoundary.pointAt(min) + normal.pointAt(normalMax) - normal.p1();
  return m_virtDisplayArea.intersected(poly);
}  // DewarpingView::virtMarginArea
}  // namespace output
