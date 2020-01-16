// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_SPFIT_MODELSHAPE_H_
#define SCANTAILOR_SPFIT_MODELSHAPE_H_

#include <QPointF>

#include "FittableSpline.h"
#include "SqDistApproximant.h"

namespace spfit {
/**
 * \brief A shape we are trying to fit a spline to.
 *
 * Could be a polyline or maybe a point cloud.
 */
class ModelShape {
 public:
  virtual ~ModelShape() = default;

  /**
   * Returns a function that approximates the squared distance to the model.
   * The function is only accurate in the neighbourhood of \p pt.
   */
  virtual SqDistApproximant localSqDistApproximant(const QPointF& pt, FittableSpline::SampleFlags flags) const = 0;
};
}  // namespace spfit
#endif
