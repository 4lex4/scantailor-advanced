// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "XSpline.h"
#include <QDebug>
#include <boost/foreach.hpp>
#include <cmath>
#include "ToLineProjector.h"
#include "VecNT.h"
#include "adiff/Function.h"
#include "adiff/SparseMap.h"

struct XSpline::TensionDerivedParams {
  static const double t0;
  static const double t1;
  static const double t2;
  static const double t3;

  // These correspond to Tk- and Tk+ in [1].
  double T0p;
  double T1p;
  double T2m;
  double T3m;

  // q parameters for GBlendFunc and HBlendFunc.
  double q[4]{};
  // p parameters for GBlendFunc.
  double p[4]{};

  TensionDerivedParams(double tension1, double tension2);
};


class XSpline::GBlendFunc {
 public:
  GBlendFunc(double q, double p);

  double value(double u) const;

  double firstDerivative(double u) const;

  double secondDerivative(double u) const;

 private:
  double m_c1;
  double m_c2;
  double m_c3;
  double m_c4;
  double m_c5;
};


class XSpline::HBlendFunc {
 public:
  explicit HBlendFunc(double q);

  double value(double u) const;

  double firstDerivative(double u) const;

  double secondDerivative(double u) const;

 private:
  double m_c1;
  double m_c2;
  double m_c4;
  double m_c5;
};


struct XSpline::DecomposedDerivs {
  double zeroDerivCoeffs[4];
  double firstDerivCoeffs[4];
  double secondDerivCoeffs[4];
  int controlPoints[4];
  int numControlPoints;

  bool hasNonZeroCoeffs(int idx) const {
    double sum = std::fabs(zeroDerivCoeffs[idx]) + std::fabs(firstDerivCoeffs[idx]) + std::fabs(secondDerivCoeffs[idx]);

    return sum > std::numeric_limits<double>::epsilon();
  }
};


int XSpline::numControlPoints() const {
  return static_cast<int>(m_controlPoints.size());
}

int XSpline::numSegments() const {
  return std::max<int>(static_cast<const int&>(m_controlPoints.size() - 1), 0);
}

double XSpline::controlPointIndexToT(int idx) const {
  assert(idx >= 0 || idx <= (int) m_controlPoints.size());

  return double(idx) / (m_controlPoints.size() - 1);
}

void XSpline::appendControlPoint(const QPointF& pos, double tension) {
  m_controlPoints.emplace_back(pos, tension);
}

void XSpline::insertControlPoint(int idx, const QPointF& pos, double tension) {
  assert(idx >= 0 || idx <= (int) m_controlPoints.size());
  m_controlPoints.insert(m_controlPoints.begin() + idx, ControlPoint(pos, tension));
}

void XSpline::eraseControlPoint(int idx) {
  assert(idx >= 0 || idx < (int) m_controlPoints.size());
  m_controlPoints.erase(m_controlPoints.begin() + idx);
}

QPointF XSpline::controlPointPosition(int idx) const {
  return m_controlPoints[idx].pos;
}

void XSpline::moveControlPoint(int idx, const QPointF& pos) {
  assert(idx >= 0 || idx < (int) m_controlPoints.size());
  m_controlPoints[idx].pos = pos;
}

double XSpline::controlPointTension(int idx) const {
  assert(idx >= 0 && idx < (int) m_controlPoints.size());

  return m_controlPoints[idx].tension;
}

void XSpline::setControlPointTension(int idx, double tension) {
  assert(idx >= 0 && idx < (int) m_controlPoints.size());
  m_controlPoints[idx].tension = tension;
}

QPointF XSpline::pointAt(double t) const {
  const int numSegments = this->numSegments();
  assert(numSegments > 0);
  assert(t >= 0 && t <= 1);

  if (t == 1.0) {
    // If we went with the branch below, we would end up with
    // segment == numSegments, which is an error.
    return pointAtImpl(numSegments - 1, 1.0);
  } else {
    const double t2 = t * numSegments;
    const double segment = std::floor(t2);

    return pointAtImpl((int) segment, t2 - segment);
  }
}

QPointF XSpline::pointAtImpl(int segment, double t) const {
  LinearCoefficient coeffs[4];
  const int numCoeffs = linearCombinationFor(coeffs, segment, t);

  QPointF pt(0, 0);
  for (int i = 0; i < numCoeffs; ++i) {
    const LinearCoefficient& c = coeffs[i];
    pt += m_controlPoints[c.controlPointIdx].pos * c.coeff;
  }

  return pt;
}

void XSpline::sample(const VirtualFunction<void, const QPointF&, double, SampleFlags>& sink,
                     const SamplingParams& params,
                     double fromT,
                     double toT) const {
  if (m_controlPoints.empty()) {
    return;
  }

  double maxSqdistToSpline = params.maxDistFromSpline;
  if (maxSqdistToSpline != NumericTraits<double>::max()) {
    maxSqdistToSpline *= params.maxDistFromSpline;
  }

  double maxSqdistBetweenSamples = params.maxDistBetweenSamples;
  if (maxSqdistBetweenSamples != NumericTraits<double>::max()) {
    maxSqdistBetweenSamples *= params.maxDistBetweenSamples;
  }

  const QPointF fromPt(pointAt(fromT));
  const QPointF toPt(pointAt(toT));
  sink(fromPt, fromT, HEAD_SAMPLE);

  const int numSegments = this->numSegments();
  if (numSegments == 0) {
    return;
  }
  const double rNumSegments = 1.0 / numSegments;

  maybeAddMoreSamples(sink, maxSqdistToSpline, maxSqdistBetweenSamples, numSegments, rNumSegments, fromT, fromPt, toT,
                      toPt);

  sink(toPt, toT, TAIL_SAMPLE);
}  // XSpline::sample

void XSpline::maybeAddMoreSamples(const VirtualFunction<void, const QPointF&, double, SampleFlags>& sink,
                                  double maxSqdistToSpline,
                                  double maxSqdistBetweenSamples,
                                  double numSegments,
                                  double rNumSegments,
                                  double prevT,
                                  const QPointF& prevPt,
                                  double nextT,
                                  const QPointF& nextPt) const {
  const double prevNextSqdist = Vec2d(nextPt - prevPt).squaredNorm();
  if (prevNextSqdist < 1e-6) {
    // Too close. Projecting anything on such a small line segment is dangerous.
    return;
  }

  SampleFlags flags = DEFAULT_SAMPLE;
  double midT = 0.5 * (prevT + nextT);
  const double nearbyJunctionT = std::floor(midT * numSegments + 0.5) * rNumSegments;

  // If nearbyJunctionT is between prevT and nextT, make it our midT.
  if (((nearbyJunctionT - prevT) * (nextT - prevT) > 0) && ((nearbyJunctionT - nextT) * (prevT - nextT) > 0)) {
    midT = nearbyJunctionT;
    flags = JUNCTION_SAMPLE;
  }

  const QPointF midPt(pointAt(midT));

  if (flags != JUNCTION_SAMPLE) {
    const QPointF projection(ToLineProjector(QLineF(prevPt, nextPt)).projectionPoint(midPt));

    if (prevNextSqdist <= maxSqdistBetweenSamples) {
      if (Vec2d(midPt - projection).squaredNorm() <= maxSqdistToSpline) {
        return;
      }
    }
  }

  maybeAddMoreSamples(sink, maxSqdistToSpline, maxSqdistBetweenSamples, numSegments, rNumSegments, prevT, prevPt, midT,
                      midPt);

  sink(midPt, midT, flags);

  maybeAddMoreSamples(sink, maxSqdistToSpline, maxSqdistBetweenSamples, numSegments, rNumSegments, midT, midPt, nextT,
                      nextPt);
}  // XSpline::maybeAddMoreSamples

void XSpline::linearCombinationAt(double t, std::vector<LinearCoefficient>& coeffs) const {
  assert(t >= 0 && t <= 1);

  const int numSegments = this->numSegments();
  assert(numSegments > 0);

  int numCoeffs = 4;
  coeffs.resize(numCoeffs);
  LinearCoefficient static_coeffs[4];

  if (t == 1.0) {
    // If we went with the branch below, we would end up with
    // segment == numSegments, which is an error.
    numCoeffs = linearCombinationFor(static_coeffs, numSegments - 1, 1.0);
  } else {
    const double t2 = t * numSegments;
    const double segment = std::floor(t2);
    numCoeffs = linearCombinationFor(static_coeffs, (int) segment, t2 - segment);
  }

  for (int i = 0; i < numCoeffs; ++i) {
    coeffs[i] = static_coeffs[i];
  }
  coeffs.resize(numCoeffs);
}

int XSpline::linearCombinationFor(LinearCoefficient* coeffs, int segment, double t) const {
  assert(segment >= 0 && segment < (int) m_controlPoints.size() - 1);
  assert(t >= 0 && t <= 1);

  int idxs[4];
  idxs[0] = std::max<int>(0, segment - 1);
  idxs[1] = segment;
  idxs[2] = segment + 1;
  idxs[3] = std::min<int>(segment + 2, static_cast<const int&>(m_controlPoints.size() - 1));

  ControlPoint pts[4];
  for (int i = 0; i < 4; ++i) {
    pts[i] = m_controlPoints[idxs[i]];
  }

  const TensionDerivedParams tdp(pts[1].tension, pts[2].tension);

  Vec4d A;

  if (t <= tdp.T0p) {
    A[0] = GBlendFunc(tdp.q[0], tdp.p[0]).value((t - tdp.T0p) / (tdp.t0 - tdp.T0p));
  } else {
    A[0] = HBlendFunc(tdp.q[0]).value((t - tdp.T0p) / (tdp.t0 - tdp.T0p));
  }

  A[1] = GBlendFunc(tdp.q[1], tdp.p[1]).value((t - tdp.T1p) / (tdp.t1 - tdp.T1p));
  A[2] = GBlendFunc(tdp.q[2], tdp.p[2]).value((t - tdp.T2m) / (tdp.t2 - tdp.T2m));

  if (t >= tdp.T3m) {
    A[3] = GBlendFunc(tdp.q[3], tdp.p[3]).value((t - tdp.T3m) / (tdp.t3 - tdp.T3m));
  } else {
    A[3] = HBlendFunc(tdp.q[3]).value((t - tdp.T3m) / (tdp.t3 - tdp.T3m));
  }

  A /= A.sum();

  int outIdx = 0;

  if (idxs[0] == idxs[1]) {
    coeffs[outIdx++] = LinearCoefficient(idxs[0], A[0] + A[1]);
  } else {
    coeffs[outIdx++] = LinearCoefficient(idxs[0], A[0]);
    coeffs[outIdx++] = LinearCoefficient(idxs[1], A[1]);
  }

  if (idxs[2] == idxs[3]) {
    coeffs[outIdx++] = LinearCoefficient(idxs[2], A[2] + A[3]);
  } else {
    coeffs[outIdx++] = LinearCoefficient(idxs[2], A[2]);
    coeffs[outIdx++] = LinearCoefficient(idxs[3], A[3]);
  }

  return outIdx;
}  // XSpline::linearCombinationFor

XSpline::PointAndDerivs XSpline::pointAndDtsAt(double t) const {
  assert(t >= 0 && t <= 1);
  PointAndDerivs pd;

  const DecomposedDerivs derivs(decomposedDerivs(t));
  for (int i = 0; i < derivs.numControlPoints; ++i) {
    const QPointF& cp = m_controlPoints[derivs.controlPoints[i]].pos;
    pd.point += cp * derivs.zeroDerivCoeffs[i];
    pd.firstDeriv += cp * derivs.firstDerivCoeffs[i];
    pd.secondDeriv += cp * derivs.secondDerivCoeffs[i];
  }

  return pd;
}

XSpline::DecomposedDerivs XSpline::decomposedDerivs(const double t) const {
  assert(t >= 0 && t <= 1);

  const int numSegments = this->numSegments();
  assert(numSegments > 0);

  if (t == 1.0) {
    // If we went with the branch below, we would end up with
    // segment == numSegments, which is an error.
    return decomposedDerivsImpl(numSegments - 1, 1.0);
  } else {
    const double t2 = t * numSegments;
    const double segment = std::floor(t2);

    return decomposedDerivsImpl((int) segment, t2 - segment);
  }
}

XSpline::DecomposedDerivs XSpline::decomposedDerivsImpl(const int segment, const double t) const {
  assert(segment >= 0 && segment < (int) m_controlPoints.size() - 1);
  assert(t >= 0 && t <= 1);

  DecomposedDerivs derivs{};

  derivs.numControlPoints = 4;  // May be modified later in this function.
  derivs.controlPoints[0] = std::max<int>(0, segment - 1);
  derivs.controlPoints[1] = segment;
  derivs.controlPoints[2] = segment + 1;
  derivs.controlPoints[3] = std::min<int>(segment + 2, static_cast<const int&>(m_controlPoints.size() - 1));

  ControlPoint pts[4];
  for (int i = 0; i < 4; ++i) {
    pts[i] = m_controlPoints[derivs.controlPoints[i]];
  }

  const TensionDerivedParams tdp(pts[1].tension, pts[2].tension);

  // Note that we don't want the derivate with respect to t that's
  // passed to us (ranging from 0 to 1 within a segment).
  // Rather we want it with respect to the t that's passed to
  // decomposedDerivs(), ranging from 0 to 1 across all segments.
  // Let's call the latter capital T.  Their relationship is:
  // t = T*numSegments - C
  // dt/dT = numSegments
  const double dtdT = this->numSegments();

  Vec4d A;
  Vec4d dA;   // First derivatives with respect to T.
  Vec4d ddA;  // Second derivatives with respect to T.
  // Control point 0.
  {
    // u = (t - tdp.T0p) / (tdp.t0 - tdp.T0p)
    const double ta = 1.0 / (tdp.t0 - tdp.T0p);
    const double tb = -tdp.T0p * ta;
    const double u = ta * t + tb;
    if (t <= tdp.T0p) {
      // u(t) = ta * tt + tb
      // u'(t) = ta
      // g(t) = g(u(t), <tension derived params>)
      GBlendFunc g(tdp.q[0], tdp.p[0]);
      A[0] = g.value(u);

      // g'(u(t(T))) = g'(u)*u'(t)*t'(T)
      dA[0] = g.firstDerivative(u) * (ta * dtdT);
      // Note that u'(t) and t'(T) are constant functions.
      // g"(u(t(T))) = g"(u)*u'(t)*t'(T)*u'(t)*t'(T)
      ddA[0] = g.secondDerivative(u) * (ta * dtdT) * (ta * dtdT);
    } else {
      HBlendFunc h(tdp.q[0]);
      A[0] = h.value(u);
      dA[0] = h.firstDerivative(u) * (ta * dtdT);
      ddA[0] = h.secondDerivative(u) * (ta * dtdT) * (ta * dtdT);
    }
  }
  // Control point 1.
  {
    // u = (t - tdp.T1p) / (tdp.t1 - tdp.T1p)
    const double ta = 1.0 / (tdp.t1 - tdp.T1p);
    const double tb = -tdp.T1p * ta;
    const double u = ta * t + tb;
    GBlendFunc g(tdp.q[1], tdp.p[1]);
    A[1] = g.value(u);
    dA[1] = g.firstDerivative(u) * (ta * dtdT);
    ddA[1] = g.secondDerivative(u) * (ta * dtdT) * (ta * dtdT);
  }

  // Control point 2.
  {
    // u = (t - tdp.T2m) / (tdp.t2 - tdp.T2m)
    const double ta = 1.0 / (tdp.t2 - tdp.T2m);
    const double tb = -tdp.T2m * ta;
    const double u = ta * t + tb;
    GBlendFunc g(tdp.q[2], tdp.p[2]);
    A[2] = g.value(u);
    dA[2] = g.firstDerivative(u) * (ta * dtdT);
    ddA[2] = g.secondDerivative(u) * (ta * dtdT) * (ta * dtdT);
  }
  // Control point 3.
  {
    // u = (t - tdp.T3m) / (tdp.t3 - tdp.T3m)
    const double ta = 1.0 / (tdp.t3 - tdp.T3m);
    const double tb = -tdp.T3m * ta;
    const double u = ta * t + tb;
    if (t >= tdp.T3m) {
      GBlendFunc g(tdp.q[3], tdp.p[3]);
      A[3] = g.value(u);
      dA[3] = g.firstDerivative(u) * (ta * dtdT);
      ddA[3] = g.secondDerivative(u) * (ta * dtdT) * (ta * dtdT);
    } else {
      HBlendFunc h(tdp.q[3]);
      A[3] = h.value(u);
      dA[3] = h.firstDerivative(u) * (ta * dtdT);
      ddA[3] = h.secondDerivative(u) * (ta * dtdT) * (ta * dtdT);
    }
  }

  const double sum = A.sum();
  const double sum2 = sum * sum;
  const double sum4 = sum2 * sum2;
  const double dSum = dA.sum();
  const double ddSum = ddA.sum();

  for (int i = 0; i < 4; ++i) {
    derivs.zeroDerivCoeffs[i] = A[i] / sum;

    const double d1 = dA[i] * sum - A[i] * dSum;
    derivs.firstDerivCoeffs[i] = d1 / sum2;  // Derivative of: A[i] / sum
    // Derivative of: dA[i] * sum
    const double dd1 = ddA[i] * sum + dA[i] * dSum;

    // Derivative of: A[i] * dSum
    const double dd2 = dA[i] * dSum + A[i] * ddSum;

    // Derivative of (dA[i] * sum - A[i] * dSum) / sum2
    const double dd3 = ((dd1 - dd2) * sum2 - d1 * (2 * sum * dSum)) / sum4;
    derivs.secondDerivCoeffs[i] = dd3;
  }
  // Merge / throw away some control points.
  int writeIdx = 0;
  int mergeIdx = 0;
  int readIdx = 1;
  const int end = 4;
  while (true) {
    assert(mergeIdx != readIdx);
    for (; readIdx != end && derivs.controlPoints[readIdx] == derivs.controlPoints[mergeIdx]; ++readIdx) {
      // Merge
      derivs.zeroDerivCoeffs[mergeIdx] += derivs.zeroDerivCoeffs[readIdx];
      derivs.firstDerivCoeffs[mergeIdx] += derivs.firstDerivCoeffs[readIdx];
      derivs.secondDerivCoeffs[mergeIdx] += derivs.secondDerivCoeffs[readIdx];
    }

    if (derivs.hasNonZeroCoeffs(mergeIdx)) {
      // Copy
      derivs.zeroDerivCoeffs[writeIdx] = derivs.zeroDerivCoeffs[mergeIdx];
      derivs.firstDerivCoeffs[writeIdx] = derivs.firstDerivCoeffs[mergeIdx];
      derivs.secondDerivCoeffs[writeIdx] = derivs.secondDerivCoeffs[mergeIdx];
      derivs.controlPoints[writeIdx] = derivs.controlPoints[mergeIdx];
      ++writeIdx;
    }

    if (readIdx == end) {
      break;
    }

    mergeIdx = readIdx;
    ++readIdx;
  }
  derivs.numControlPoints = writeIdx;

  return derivs;
}  // XSpline::decomposedDerivsImpl

QuadraticFunction XSpline::controlPointsAttractionForce() const {
  return controlPointsAttractionForce(0, this->numSegments());
}

QuadraticFunction XSpline::controlPointsAttractionForce(int segBegin, int segEnd) const {
  using namespace adiff;

  assert(segBegin >= 0 && segBegin <= this->numSegments());
  assert(segEnd >= 0 && segEnd <= this->numSegments());
  assert(segBegin <= segEnd);

  const int numControlPoints = this->numControlPoints();

  SparseMap<2> sparseMap(numControlPoints * 2);
  sparseMap.markAllNonZero();

  Function<2> force(sparseMap);
  if (segBegin != segEnd) {
    Function<2> prevX(segBegin * 2, m_controlPoints[segBegin].pos.x(), sparseMap);
    Function<2> prevY(segBegin * 2 + 1, m_controlPoints[segBegin].pos.y(), sparseMap);

    for (int i = segBegin + 1; i <= segEnd; ++i) {
      Function<2> nextX(i * 2, m_controlPoints[i].pos.x(), sparseMap);
      Function<2> nextY(i * 2 + 1, m_controlPoints[i].pos.y(), sparseMap);

      const Function<2> dx(nextX - prevX);
      const Function<2> dy(nextY - prevY);
      force += dx * dx + dy * dy;

      nextX.swap(prevX);
      nextY.swap(prevY);
    }
  }

  QuadraticFunction f(numControlPoints * 2);
  f.A = 0.5 * force.hessian(sparseMap);
  f.b = force.gradient(sparseMap);
  f.c = force.value;

  return f;
}  // XSpline::controlPointsAttractionForce

QuadraticFunction XSpline::junctionPointsAttractionForce() const {
  return junctionPointsAttractionForce(0, this->numSegments());
}

QuadraticFunction XSpline::junctionPointsAttractionForce(int segBegin, int segEnd) const {
  using namespace adiff;

  assert(segBegin >= 0 && segBegin <= this->numSegments());
  assert(segEnd >= 0 && segEnd <= this->numSegments());
  assert(segBegin <= segEnd);

  const int numControlPoints = this->numControlPoints();

  SparseMap<2> sparseMap(numControlPoints * 2);
  sparseMap.markAllNonZero();

  Function<2> force(sparseMap);

  if (segBegin != segEnd) {
    QPointF pt(pointAt(controlPointIndexToT(segBegin)));
    std::vector<LinearCoefficient> coeffs;
    Function<2> prevX(0);
    Function<2> prevY(0);

    for (int i = segBegin; i <= segEnd; ++i) {
      Function<2> nextX(sparseMap);
      Function<2> nextY(sparseMap);

      linearCombinationAt(controlPointIndexToT(i), coeffs);
      for (const LinearCoefficient& coeff : coeffs) {
        const QPointF cp(m_controlPoints[coeff.controlPointIdx].pos);
        Function<2> x(coeff.controlPointIdx * 2, cp.x(), sparseMap);
        Function<2> y(coeff.controlPointIdx * 2 + 1, cp.y(), sparseMap);
        x *= coeff.coeff;
        y *= coeff.coeff;
        nextX += x;
        nextY += y;
      }

      if (i != segBegin) {
        const Function<2> dx(nextX - prevX);
        const Function<2> dy(nextY - prevY);
        force += dx * dx + dy * dy;
      }

      nextX.swap(prevX);
      nextY.swap(prevY);
    }
  }

  QuadraticFunction f(numControlPoints * 2);
  f.A = 0.5 * force.hessian(sparseMap);
  f.b = force.gradient(sparseMap);
  f.c = force.value;

  return f;
}  // XSpline::junctionPointsAttractionForce

QPointF XSpline::pointClosestTo(const QPointF to, double* t, double accuracy) const {
  if (m_controlPoints.empty()) {
    if (t) {
      *t = 0;
    }

    return QPointF();
  }

  const int numSegments = this->numSegments();
  if (numSegments == 0) {
    if (t) {
      *t = 0;
    }

    return m_controlPoints.front().pos;
  }

  QPointF prevPt(pointAtImpl(0, 0));
  QPointF nextPt;
  // Find the closest segment.
  int bestSegment = 0;
  double bestSqdist = Vec2d(to - prevPt).squaredNorm();
  for (int seg = 0; seg < numSegments; ++seg, prevPt = nextPt) {
    nextPt = pointAtImpl(seg, 1.0);

    const double sqdist = sqDistToLine(to, QLineF(prevPt, nextPt));
    if (sqdist < bestSqdist) {
      bestSegment = seg;
      bestSqdist = sqdist;
    }
  }
  // Continue with a binary search.
  const double sqAccuracy = accuracy * accuracy;
  double prevT = 0;
  double nextT = 1;
  prevPt = pointAtImpl(bestSegment, prevT);
  nextPt = pointAtImpl(bestSegment, nextT);

  while (Vec2d(prevPt - nextPt).squaredNorm() > sqAccuracy) {
    const double midT = 0.5 * (prevT + nextT);
    const QPointF midPt(pointAtImpl(bestSegment, midT));

    const ToLineProjector projector(QLineF(prevPt, nextPt));
    const double pt = projector.projectionScalar(to);
    const double pm = projector.projectionScalar(midPt);
    if (pt < pm) {
      nextT = midT;
      nextPt = midPt;
    } else {
      prevT = midT;
      prevPt = midPt;
    }
  }

  // Take the closest of prevPt and nextPt.
  if (Vec2d(to - prevPt).squaredNorm() < Vec2d(to - nextPt).squaredNorm()) {
    if (t) {
      *t = (bestSegment + prevT) / numSegments;
    }

    return prevPt;
  } else {
    if (t) {
      *t = (bestSegment + nextT) / numSegments;
    }

    return nextPt;
  }
}  // XSpline::pointClosestTo

QPointF XSpline::pointClosestTo(const QPointF to, double accuracy) const {
  return pointClosestTo(to, nullptr, accuracy);
}

std::vector<QPointF> XSpline::toPolyline(const SamplingParams& params, double fromT, double toT) const {
  std::vector<QPointF> polyline;

  auto sink = [&polyline](const QPointF& pt, double, SampleFlags) { polyline.push_back(pt); };
  sample(ProxyFunction<decltype(sink), void, const QPointF&, double, SampleFlags>(sink), params, fromT, toT);

  return polyline;
}

double XSpline::sqDistToLine(const QPointF& pt, const QLineF& line) {
  const ToLineProjector projector(line);
  const double p = projector.projectionScalar(pt);
  QPointF ppt;
  if (p <= 0) {
    ppt = line.p1();
  } else if (p >= 1) {
    ppt = line.p2();
  } else {
    ppt = line.pointAt(p);
  }

  return Vec2d(pt - ppt).squaredNorm();
}

/*===================== TensionDerivedParams =====================*/
// We make t lie in [0 .. 1] within a segment,
// which gives us delta == 1 and the following tk's:
const double XSpline::TensionDerivedParams::t0 = -1;
const double XSpline::TensionDerivedParams::t1 = 0;
const double XSpline::TensionDerivedParams::t2 = 1;
const double XSpline::TensionDerivedParams::t3 = 2;

static double square(double v) {
  return v * v;
}

XSpline::TensionDerivedParams::TensionDerivedParams(const double tension1, const double tension2) {
  // tension1, tension2 lie in [-1 .. 1]

  // Tk+ = t(k+1) + s(k+1)
  // Tk- = t(k-1) - s(k-1)
  const double s1 = std::max<double>(tension1, 0);
  const double s2 = std::max<double>(tension2, 0);
  T0p = t1 + s1;
  T1p = t2 + s2;
  T2m = t1 - s1;
  T3m = t2 - s2;

  // q's lie in [0 .. 0.5]
  const double s1_ = -0.5 * std::min<double>(tension1, 0);
  const double s2_ = -0.5 * std::min<double>(tension2, 0);
  q[0] = s1_;
  q[1] = s2_;
  q[2] = s1_;
  q[3] = s2_;
  // Formula 17 in [1]:
  p[0] = 2.0 * square(t0 - T0p);
  p[1] = 2.0 * square(t1 - T1p);
  p[2] = 2.0 * square(t2 - T2m);
  p[3] = 2.0 * square(t3 - T3m);
}

/*========================== GBlendFunc ==========================*/

XSpline::GBlendFunc::GBlendFunc(double q, double p)
    : m_c1(q),
      // See formula 20 in [1].
      m_c2(2 * q),
      m_c3(10 - 12 * q - p),
      m_c4(2 * p + 14 * q - 15),
      m_c5(6 - 5 * q - p) {}

double XSpline::GBlendFunc::value(double u) const {
  const double u2 = u * u;
  const double u3 = u2 * u;
  const double u4 = u3 * u;
  const double u5 = u4 * u;

  return m_c1 * u + m_c2 * u2 + m_c3 * u3 + m_c4 * u4 + m_c5 * u5;
}

double XSpline::GBlendFunc::firstDerivative(double u) const {
  const double u2 = u * u;
  const double u3 = u2 * u;
  const double u4 = u3 * u;

  return m_c1 + 2 * m_c2 * u + 3 * m_c3 * u2 + 4 * m_c4 * u3 + 5 * m_c5 * u4;
}

double XSpline::GBlendFunc::secondDerivative(double u) const {
  const double u2 = u * u;
  const double u3 = u2 * u;

  return 2 * m_c2 + 6 * m_c3 * u + 12 * m_c4 * u2 + 20 * m_c5 * u3;
}

/*========================== HBlendFunc ==========================*/

XSpline::HBlendFunc::HBlendFunc(double q)
    : m_c1(q),
      // See formula 20 in [1].
      m_c2(2 * q),
      m_c4(-2 * q),
      m_c5(-q) {}

double XSpline::HBlendFunc::value(double u) const {
  const double u2 = u * u;
  const double u3 = u2 * u;
  const double u4 = u3 * u;
  const double u5 = u4 * u;

  return m_c1 * u + m_c2 * u2 + m_c4 * u4 + m_c5 * u5;
}

double XSpline::HBlendFunc::firstDerivative(double u) const {
  const double u2 = u * u;
  const double u3 = u2 * u;
  const double u4 = u3 * u;

  return m_c1 + 2 * m_c2 * u + 4 * m_c4 * u3 + 5 * m_c5 * u4;
}

double XSpline::HBlendFunc::secondDerivative(double u) const {
  const double u2 = u * u;
  const double u3 = u2 * u;

  return 2 * m_c2 + 12 * m_c4 * u2 + 20 * m_c5 * u3;
}

/*======================== PointAndDerivs ========================*/

double XSpline::PointAndDerivs::signedCurvature() const {
  const double cross = firstDeriv.x() * secondDeriv.y() - firstDeriv.y() * secondDeriv.x();
  double tlen = std::sqrt(firstDeriv.x() * firstDeriv.x() + firstDeriv.y() * firstDeriv.y());

  return cross / (tlen * tlen * tlen);
}
