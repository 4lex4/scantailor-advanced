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

#include "PolylineModelShape.h"
#include <QDebug>
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

  const int num_control_points = spline.numControlPoints();
  const double scale = 1.0 / (num_control_points - 1);
  for (int i = 0; i < num_control_points; ++i) {
    m_vertices.push_back(spline.pointAndDtsAt(i * scale));
  }
}

SqDistApproximant PolylineModelShape::localSqDistApproximant(const QPointF& pt,
                                                             FittableSpline::SampleFlags sample_flags) const {
  if (m_vertices.empty()) {
    return SqDistApproximant();
  }

  // First, find the point on the polyline closest to pt.
  QPointF best_foot_point;
  double best_sqdist = NumericTraits<double>::max();
  double segment_t = -1;
  int segment_idx = -1;  // If best_foot_point is on a segment, its index goes here.
  int vertex_idx = -1;   // If best_foot_point is a vertex, its index goes here.
  // Project pt to each segment.
  const int num_segments = int(m_vertices.size()) - 1;
  for (int i = 0; i < num_segments; ++i) {
    const QPointF pt1(m_vertices[i].point);
    const QPointF pt2(m_vertices[i + 1].point);
    const QLineF segment(pt1, pt2);
    const double s = ToLineProjector(segment).projectionScalar(pt);
    if ((s > 0) && (s < 1)) {
      const QPointF foot_point(segment.pointAt(s));
      const Vec2d vec(pt - foot_point);
      const double sqdist = vec.squaredNorm();
      if (sqdist < best_sqdist) {
        best_sqdist = sqdist;
        best_foot_point = foot_point;
        segment_idx = i;
        segment_t = s;
        vertex_idx = -1;
      }
    }
  }
  // Check if pt is closer to a vertex than to any segment.
  const auto num_points = static_cast<const int>(m_vertices.size());
  for (int i = 0; i < num_points; ++i) {
    const QPointF vtx(m_vertices[i].point);
    const Vec2d vec(pt - vtx);
    const double sqdist = vec.squaredNorm();
    if (sqdist < best_sqdist) {
      best_sqdist = sqdist;
      best_foot_point = vtx;
      vertex_idx = i;
      segment_idx = -1;
    }
  }

  if (segment_idx != -1) {
    // The foot point is on a line segment.
    assert(segment_t >= 0 && segment_t <= 1);

    const XSpline::PointAndDerivs& pd1 = m_vertices[segment_idx];
    const XSpline::PointAndDerivs& pd2 = m_vertices[segment_idx + 1];
    const FrenetFrame frenet_frame(best_foot_point, pd2.point - pd1.point);

    const double k1 = pd1.signedCurvature();
    const double k2 = pd2.signedCurvature();
    const double weighted_k = k1 + segment_t * (k2 - k1);

    return calcApproximant(pt, sample_flags, DEFAULT_FLAGS, frenet_frame, weighted_k);
  } else {
    // The foot point is a vertex of the polyline.
    assert(vertex_idx != -1);

    const XSpline::PointAndDerivs& pd = m_vertices[vertex_idx];
    const FrenetFrame frenet_frame(best_foot_point, pd.firstDeriv);

    Flags polyline_flags = DEFAULT_FLAGS;
    if (vertex_idx == 0) {
      polyline_flags |= POLYLINE_FRONT;
    }
    if (vertex_idx == int(m_vertices.size()) - 1) {
      polyline_flags |= POLYLINE_BACK;
    }

    return calcApproximant(pt, sample_flags, polyline_flags, frenet_frame, pd.signedCurvature());
  }
}  // PolylineModelShape::localSqDistApproximant

SqDistApproximant PolylineModelShape::calcApproximant(const QPointF& pt,
                                                      const FittableSpline::SampleFlags sample_flags,
                                                      const Flags polyline_flags,
                                                      const FrenetFrame& frenet_frame,
                                                      const double signed_curvature) const {
  if (sample_flags & (FittableSpline::HEAD_SAMPLE | FittableSpline::TAIL_SAMPLE)) {
    return SqDistApproximant::pointDistance(frenet_frame.origin());
  } else {
    return SqDistApproximant::curveDistance(pt, frenet_frame, signed_curvature);
  }
}
}  // namespace spfit