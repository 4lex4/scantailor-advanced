// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "PolylineModelShape.h"

#include <QDebug>
#include <stdexcept>

#include "FrenetFrame.h"
#include "ToLineProjector.h"

namespace spfit {
PolylineModelShape::PolylineModelShape(const std::vector<QPointF>& polyline) {
  if (polyline.size() <= 1) {
    throw std::invalid_argument("PolylineModelShape: polyline must have at least 2 vertices");
  }

  // We build an interpolating X-spline with control points at the vertices
  // of our polyline.  We'll use it to calculate curvature at polyline vertices.
  XSpline spline;

  for (const QPointF& pt : polyline) {
    spline.appendControlPoint(pt, -1);
  }

  const int numControlPoints = spline.numControlPoints();
  const double scale = 1.0 / (numControlPoints - 1);
  for (int i = 0; i < numControlPoints; ++i) {
    m_vertices.push_back(spline.pointAndDtsAt(i * scale));
  }
}

SqDistApproximant PolylineModelShape::localSqDistApproximant(const QPointF& pt,
                                                             FittableSpline::SampleFlags sampleFlags) const {
  if (m_vertices.empty()) {
    return SqDistApproximant();
  }

  // First, find the point on the polyline closest to pt.
  QPointF bestFootPoint;
  double bestSqdist = NumericTraits<double>::max();
  double segmentT = -1;
  int segmentIdx = -1;  // If bestFootPoint is on a segment, its index goes here.
  int vertexIdx = -1;   // If bestFootPoint is a vertex, its index goes here.
  // Project pt to each segment.
  const int numSegments = int(m_vertices.size()) - 1;
  for (int i = 0; i < numSegments; ++i) {
    const QPointF pt1(m_vertices[i].point);
    const QPointF pt2(m_vertices[i + 1].point);
    const QLineF segment(pt1, pt2);
    const double s = ToLineProjector(segment).projectionScalar(pt);
    if ((s > 0) && (s < 1)) {
      const QPointF footPoint(segment.pointAt(s));
      const Vec2d vec(pt - footPoint);
      const double sqdist = vec.squaredNorm();
      if (sqdist < bestSqdist) {
        bestSqdist = sqdist;
        bestFootPoint = footPoint;
        segmentIdx = i;
        segmentT = s;
        vertexIdx = -1;
      }
    }
  }
  // Check if pt is closer to a vertex than to any segment.
  const auto numPoints = static_cast<int>(m_vertices.size());
  for (int i = 0; i < numPoints; ++i) {
    const QPointF vtx(m_vertices[i].point);
    const Vec2d vec(pt - vtx);
    const double sqdist = vec.squaredNorm();
    if (sqdist < bestSqdist) {
      bestSqdist = sqdist;
      bestFootPoint = vtx;
      vertexIdx = i;
      segmentIdx = -1;
    }
  }

  if (segmentIdx != -1) {
    // The foot point is on a line segment.
    assert(segmentT >= 0 && segmentT <= 1);

    const XSpline::PointAndDerivs& pd1 = m_vertices[segmentIdx];
    const XSpline::PointAndDerivs& pd2 = m_vertices[segmentIdx + 1];
    const FrenetFrame frenetFrame(bestFootPoint, pd2.point - pd1.point);

    const double k1 = pd1.signedCurvature();
    const double k2 = pd2.signedCurvature();
    const double weightedK = k1 + segmentT * (k2 - k1);
    return calcApproximant(pt, sampleFlags, DEFAULT_FLAGS, frenetFrame, weightedK);
  } else {
    // The foot point is a vertex of the polyline.
    assert(vertexIdx != -1);

    const XSpline::PointAndDerivs& pd = m_vertices[vertexIdx];
    const FrenetFrame frenetFrame(bestFootPoint, pd.firstDeriv);

    Flags polylineFlags = DEFAULT_FLAGS;
    if (vertexIdx == 0) {
      polylineFlags |= POLYLINE_FRONT;
    }
    if (vertexIdx == int(m_vertices.size()) - 1) {
      polylineFlags |= POLYLINE_BACK;
    }
    return calcApproximant(pt, sampleFlags, polylineFlags, frenetFrame, pd.signedCurvature());
  }
}  // PolylineModelShape::localSqDistApproximant

SqDistApproximant PolylineModelShape::calcApproximant(const QPointF& pt,
                                                      const FittableSpline::SampleFlags sampleFlags,
                                                      const Flags polylineFlags,
                                                      const FrenetFrame& frenetFrame,
                                                      const double signedCurvature) const {
  if (sampleFlags & (FittableSpline::HEAD_SAMPLE | FittableSpline::TAIL_SAMPLE)) {
    return SqDistApproximant::pointDistance(frenetFrame.origin());
  } else {
    return SqDistApproximant::curveDistance(pt, frenetFrame, signedCurvature);
  }
}
}  // namespace spfit
