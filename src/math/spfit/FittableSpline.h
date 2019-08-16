// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SPFIT_FITTABLE_SPLINE_H_
#define SPFIT_FITTABLE_SPLINE_H_

#include <QPointF>
#include <vector>
#include "FlagOps.h"
#include "NumericTraits.h"
#include "VirtualFunction.h"

namespace spfit {
/**
 * \brief Implementing this interface allows a spline to be fitted to a polyline.
 */
class FittableSpline {
 public:
  enum SampleFlags {
    DEFAULT_SAMPLE = 0,
    HEAD_SAMPLE = 1 << 0,    /**< Start point of an open spline. */
    TAIL_SAMPLE = 1 << 1,    /**< End point of an open spline. */
    JUNCTION_SAMPLE = 1 << 2 /**< Point on the boundary of two segments. */
  };

  /**
   * For a spline to be fittable, any point on a spline must be representable
   * as a linear combination of spline's control points.  The linear coefficients
   * will of course depend on parameter t, and this dependency doesn't have to be
   * linear.
   *
   * This class represents a single linear coefficient assiciated with
   * a particular control point.
   */
  struct LinearCoefficient {
    double coeff;
    int controlPointIdx;

    LinearCoefficient() : coeff(0), controlPointIdx(-1) {}

    LinearCoefficient(int cp_idx, double cf) : coeff(cf), controlPointIdx(cp_idx) {}
  };

  struct SamplingParams {
    /**
     * The maximum distance from any point on the polyline that's the
     * result of sampling to the spline.
     */
    double maxDistFromSpline;

    /**
     * The maximum distance between two adjacent samples.
     */
    double maxDistBetweenSamples;

    explicit SamplingParams(double max_dist_from_spline = 0.2,
                            double max_dist_between_samples = NumericTraits<double>::max())
        : maxDistFromSpline(max_dist_from_spline), maxDistBetweenSamples(max_dist_between_samples) {}
  };

  virtual ~FittableSpline() = default;

  virtual int numControlPoints() const = 0;

  virtual QPointF controlPointPosition(int idx) const = 0;

  virtual void moveControlPoint(int idx, const QPointF& pos) = 0;

  /**
   * \brief For a given t, calculates a linear combination of control points that result
   *        in a point on the spline corresponding to the given t.
   *
   * \param t Position on the spline.  The range of t is [0, 1].
   * \param coeffs The vector to write linear coefficients into.  Existing contents
   *        (if any) will be discarded.  Implementations must make sure that at most
   *        one coefficient is being produced for each control point.
   */
  virtual void linearCombinationAt(double t, std::vector<LinearCoefficient>& coeffs) const = 0;

  /**
   * \brief Generates an ordered set of points on a spline.
   *
   * \p sink will be called with the following arguments:
   * -# Point on the spline.
   * -# t value corresponding to that point.
   * -# SampleFlags for the point.
   *
   * \note No matter the values of from_t and to_t, samples
   *       corresponding to them will be marked with HEAD_SAMPLE
   *       and TAIL_SAMPLE respectably.
   */
  virtual void sample(const VirtualFunction<void, const QPointF&, double, SampleFlags>& sink,
                      const SamplingParams& params,
                      double from_t = 0.0,
                      double to_t = 1.0) const = 0;
};


DEFINE_FLAG_OPS(FittableSpline::SampleFlags)
}  // namespace spfit
#endif  // ifndef SPFIT_FITTABLE_SPLINE_H_
