// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SPFIT_POLYLINE_MODEL_SHAPE_H_
#define SPFIT_POLYLINE_MODEL_SHAPE_H_

#include <QPointF>
#include <vector>
#include "FlagOps.h"
#include "ModelShape.h"
#include "NonCopyable.h"
#include "SqDistApproximant.h"
#include "VecNT.h"
#include "XSpline.h"

namespace spfit {
class PolylineModelShape : public ModelShape {
  DECLARE_NON_COPYABLE(PolylineModelShape)

 public:
  enum Flags { DEFAULT_FLAGS = 0, POLYLINE_FRONT = 1 << 0, POLYLINE_BACK = 1 << 1 };

  explicit PolylineModelShape(const std::vector<QPointF>& polyline);

  SqDistApproximant localSqDistApproximant(const QPointF& pt, FittableSpline::SampleFlags sample_flags) const override;

 protected:
  virtual SqDistApproximant calcApproximant(const QPointF& pt,
                                            FittableSpline::SampleFlags sample_flags,
                                            Flags polyline_flags,
                                            const FrenetFrame& frenet_frame,
                                            double signed_curvature) const;

 private:
  std::vector<XSpline::PointAndDerivs> m_vertices;
};


DEFINE_FLAG_OPS(PolylineModelShape::Flags)
}  // namespace spfit
#endif  // ifndef SPFIT_POLYLINE_MODEL_SHAPE_H_
