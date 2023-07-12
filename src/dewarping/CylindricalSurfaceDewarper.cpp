// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "CylindricalSurfaceDewarper.h"

#include <QDebug>
#include <boost/foreach.hpp>

#include "NumericTraits.h"
#include "ToLineProjector.h"

/*
   Naming conventions:
   img: Coordinates in the warped image.
   pln: Coordinates on a plane where the 4 corner points of the curved
     quadrilateral are supposed to lie.  In our model we assume that
     all 4 lie on the same plane.  The corner points are mapped to
     the following points on the plane:
 * Start point of curve1 [top curve]: (0, 0)
 * End point of curve1 [top curve]: (1, 0)
 * Start point of curve2 [bottom curve]: (0, 1)
 * End point of curve2 [bottom curve]: (1, 1)
     pln and img coordinates are linked by a 2D homography,
     namely m_pln2img and m_img2pln.
   crv: Dewarped normalized coordinates.  crv X coordinates are linked
     to pln X ccoordinates through m_arcLengthMapper while the Y
     coordinates are linked by a one dimensional homography that's
     different for each generatrix.
 */

namespace dewarping {
class CylindricalSurfaceDewarper::CoupledPolylinesIterator {
 public:
  CoupledPolylinesIterator(const std::vector<QPointF>& imgDirectrix1,
                           const std::vector<QPointF>& imgDirectrix2,
                           const HomographicTransform<2, double>& pln2img,
                           const HomographicTransform<2, double>& img2pln);

  bool next(QPointF& imgPt1, QPointF& imgPt2, double& plnX);

 private:
  void next1(QPointF& imgPt1, QPointF& imgPt2, double& plnX);

  void next2(QPointF& imgPt1, QPointF& imgPt2, double& plnX);

  void advance1();

  void advance2();

  std::vector<QPointF>::const_iterator m_seq1It;
  std::vector<QPointF>::const_iterator m_seq2It;
  std::vector<QPointF>::const_iterator m_seq1End;
  std::vector<QPointF>::const_iterator m_seq2End;
  HomographicTransform<2, double> m_pln2img;
  HomographicTransform<2, double> m_img2pln;
  Vec2d m_prevImgPt1;
  Vec2d m_prevImgPt2;
  Vec2d m_nextImgPt1;
  Vec2d m_nextImgPt2;
  double m_nextPlnX1;
  double m_nextPlnX2;
};


CylindricalSurfaceDewarper::CylindricalSurfaceDewarper(const std::vector<QPointF>& imgDirectrix1,
                                                       const std::vector<QPointF>& imgDirectrix2,
                                                       double depthPerception)
    : m_pln2img(calcPlnToImgHomography(imgDirectrix1, imgDirectrix2)),
      m_img2pln(m_pln2img.inv()),
      m_depthPerception(depthPerception),
      m_plnStraightLineY(calcPlnStraightLineY(imgDirectrix1, imgDirectrix2, m_pln2img, m_img2pln)),
      m_directrixArcLength(1.0),
      m_imgDirectrix1Intersector(imgDirectrix1),
      m_imgDirectrix2Intersector(imgDirectrix2) {
  initArcLengthMapper(imgDirectrix1, imgDirectrix2);
}

CylindricalSurfaceDewarper::Generatrix CylindricalSurfaceDewarper::mapGeneratrix(double crvX, State& state) const {
  const double plnX = m_arcLengthMapper.arcLenToX(crvX, state.m_arcLengthHint);

  const Vec2d plnTopPt(plnX, 0);
  const Vec2d plnBottomPt(plnX, 1);
  const Vec2d imgTopPt(m_pln2img(plnTopPt));
  const Vec2d imgBottomPt(m_pln2img(plnBottomPt));
  const QLineF imgGeneratrix(imgTopPt, imgBottomPt);
  const ToLineProjector projector(imgGeneratrix);
  const Vec2d imgDirectrix1Pt(m_imgDirectrix1Intersector.intersect(imgGeneratrix, state.m_intersectionHint1));
  const Vec2d imgDirectrix2Pt(m_imgDirectrix2Intersector.intersect(imgGeneratrix, state.m_intersectionHint2));
  const Vec2d imgStraightLinePt(m_pln2img(Vec2d(plnX, m_plnStraightLineY)));
  const double imgDirectrix1Proj(projector.projectionScalar(imgDirectrix1Pt));
  const double imgDirectrix2Proj(projector.projectionScalar(imgDirectrix2Pt));
  const double imgStraightLineProj(projector.projectionScalar(imgStraightLinePt));

  boost::array<std::pair<double, double>, 3> pairs;
  pairs[0] = std::make_pair(0.0, imgDirectrix1Proj);
  pairs[1] = std::make_pair(1.0, imgDirectrix2Proj);
  if ((std::fabs(m_plnStraightLineY) < 0.05) || (std::fabs(m_plnStraightLineY - 1.0) < 0.05)) {
    pairs[2] = std::make_pair(0.5, 0.5 * (imgDirectrix1Proj + imgDirectrix2Proj));
  } else {
    pairs[2] = std::make_pair(m_plnStraightLineY, imgStraightLineProj);
  }
  HomographicTransform<1, double> H(threePoint1DHomography(pairs));
  return Generatrix(imgGeneratrix, H);
}  // CylindricalSurfaceDewarper::mapGeneratrix

QPointF CylindricalSurfaceDewarper::mapToDewarpedSpace(const QPointF& imgPt) const {
  State state;

  const double plnX = m_img2pln(imgPt)[0];
  const double crvX = m_arcLengthMapper.xToArcLen(plnX, state.m_arcLengthHint);

  const Vec2d plnTopPt(plnX, 0);
  const Vec2d plnBottomPt(plnX, 1);
  const Vec2d imgTopPt(m_pln2img(plnTopPt));
  const Vec2d imgBottomPt(m_pln2img(plnBottomPt));
  const QLineF imgGeneratrix(imgTopPt, imgBottomPt);
  const ToLineProjector projector(imgGeneratrix);
  const Vec2d imgDirectrix1Pt(m_imgDirectrix1Intersector.intersect(imgGeneratrix, state.m_intersectionHint1));
  const Vec2d imgDirectrix2Pt(m_imgDirectrix2Intersector.intersect(imgGeneratrix, state.m_intersectionHint2));
  const Vec2d imgStraightLinePt(m_pln2img(Vec2d(plnX, m_plnStraightLineY)));
  const double imgDirectrix1Proj(projector.projectionScalar(imgDirectrix1Pt));
  const double imgDirectrix2Proj(projector.projectionScalar(imgDirectrix2Pt));
  const double imgStraightLineProj(projector.projectionScalar(imgStraightLinePt));

  boost::array<std::pair<double, double>, 3> pairs;
  pairs[0] = std::make_pair(imgDirectrix1Proj, 0.0);
  pairs[1] = std::make_pair(imgDirectrix2Proj, 1.0);
  if ((std::fabs(m_plnStraightLineY) < 0.05) || (std::fabs(m_plnStraightLineY - 1.0) < 0.05)) {
    pairs[2] = std::make_pair(0.5 * (imgDirectrix1Proj + imgDirectrix2Proj), 0.5);
  } else {
    pairs[2] = std::make_pair(imgStraightLineProj, m_plnStraightLineY);
  }
  const HomographicTransform<1, double> H(threePoint1DHomography(pairs));

  const double imgPtProj(projector.projectionScalar(imgPt));
  const double crvY = H(imgPtProj);
  return QPointF(crvX, crvY);
}  // CylindricalSurfaceDewarper::mapToDewarpedSpace

QPointF CylindricalSurfaceDewarper::mapToWarpedSpace(const QPointF& crvPt) const {
  State state;
  const Generatrix gtx(mapGeneratrix(crvPt.x(), state));
  return gtx.imgLine.pointAt(gtx.pln2img(crvPt.y()));
}

HomographicTransform<2, double> CylindricalSurfaceDewarper::calcPlnToImgHomography(
    const std::vector<QPointF>& imgDirectrix1,
    const std::vector<QPointF>& imgDirectrix2) {
  boost::array<std::pair<QPointF, QPointF>, 4> pairs;
  pairs[0] = std::make_pair(QPointF(0, 0), imgDirectrix1.front());
  pairs[1] = std::make_pair(QPointF(1, 0), imgDirectrix1.back());
  pairs[2] = std::make_pair(QPointF(0, 1), imgDirectrix2.front());
  pairs[3] = std::make_pair(QPointF(1, 1), imgDirectrix2.back());
  return fourPoint2DHomography(pairs);
}

double CylindricalSurfaceDewarper::calcPlnStraightLineY(const std::vector<QPointF>& imgDirectrix1,
                                                        const std::vector<QPointF>& imgDirectrix2,
                                                        const HomographicTransform<2, double> pln2img,
                                                        const HomographicTransform<2, double> img2pln) {
  double plnYAccum = 0;
  double weightAccum = 0;

  CoupledPolylinesIterator it(imgDirectrix1, imgDirectrix2, pln2img, img2pln);
  QPointF imgCurve1Pt;
  QPointF imgCurve2Pt;
  double plnX;
  while (it.next(imgCurve1Pt, imgCurve2Pt, plnX)) {
    const QLineF imgGeneratrix(imgCurve1Pt, imgCurve2Pt);
    const Vec2d imgLine1Pt(pln2img(Vec2d(plnX, 0)));
    const Vec2d imgLine2Pt(pln2img(Vec2d(plnX, 1)));
    const ToLineProjector projector(imgGeneratrix);
    const double p1 = 0;
    const double p2 = projector.projectionScalar(imgLine1Pt);
    const double p3 = projector.projectionScalar(imgLine2Pt);
    const double p4 = 1;
    const double dp1 = p2 - p1;
    const double dp2 = p4 - p3;
    const double weight = std::fabs(dp1 + dp2);
    if (weight < 0.01) {
      continue;
    }

    const double p0 = (p3 * dp1 + p2 * dp2) / (dp1 + dp2);
    const Vec2d imgPt(imgGeneratrix.pointAt(p0));
    plnYAccum += img2pln(imgPt)[1] * weight;
    weightAccum += weight;
  }
  return weightAccum == 0 ? 0.5 : plnYAccum / weightAccum;
}  // CylindricalSurfaceDewarper::calcPlnStraightLineY

HomographicTransform<2, double> CylindricalSurfaceDewarper::fourPoint2DHomography(
    const boost::array<std::pair<QPointF, QPointF>, 4>& pairs) {
  VecNT<64, double> A;
  VecNT<8, double> B;
  double* pa = A.data();
  double* pb = B.data();

  using Pair = std::pair<QPointF, QPointF>;
  for (const Pair& pair : pairs) {
    const QPointF from(pair.first);
    const QPointF to(pair.second);

    pa[8 * 0] = -from.x();
    pa[8 * 1] = -from.y();
    pa[8 * 2] = -1;
    pa[8 * 3] = 0;
    pa[8 * 4] = 0;
    pa[8 * 5] = 0;
    pa[8 * 6] = to.x() * from.x();
    pa[8 * 7] = to.x() * from.y();
    pb[0] = -to.x();
    ++pa;
    ++pb;

    pa[8 * 0] = 0;
    pa[8 * 1] = 0;
    pa[8 * 2] = 0;
    pa[8 * 3] = -from.x();
    pa[8 * 4] = -from.y();
    pa[8 * 5] = -1;
    pa[8 * 6] = to.y() * from.x();
    pa[8 * 7] = to.y() * from.y();
    pb[0] = -to.y();
    ++pa;
    ++pb;
  }

  VecNT<9, double> H;
  H[8] = 1.0;

  MatrixCalc<double> mc;
  mc(A, 8, 8).solve(mc(B, 8, 1)).write(H);
  mc(H, 3, 3).trans().write(H);
  return HomographicTransform<2, double>(H);
}  // CylindricalSurfaceDewarper::fourPoint2DHomography

HomographicTransform<1, double> CylindricalSurfaceDewarper::threePoint1DHomography(
    const boost::array<std::pair<double, double>, 3>& pairs) {
  VecNT<9, double> A;
  VecNT<3, double> B;
  double* pa = A.data();
  double* pb = B.data();

  using Pair = std::pair<double, double>;
  for (const Pair& pair : pairs) {
    const double from = pair.first;
    const double to = pair.second;

    pa[3 * 0] = -from;
    pa[3 * 1] = -1;
    pa[3 * 2] = from * to;
    pb[0] = -to;
    ++pa;
    ++pb;
  }

  Vec4d H;
  H[3] = 1.0;

  MatrixCalc<double> mc;
  mc(A, 3, 3).solve(mc(B, 3, 1)).write(H);
  mc(H, 2, 2).trans().write(H);
  return HomographicTransform<1, double>(H);
}

void CylindricalSurfaceDewarper::initArcLengthMapper(const std::vector<QPointF>& imgDirectrix1,
                                                     const std::vector<QPointF>& imgDirectrix2) {
  double prevElevation = 0;
  CoupledPolylinesIterator it(imgDirectrix1, imgDirectrix2, m_pln2img, m_img2pln);
  QPointF imgCurve1Pt;
  QPointF imgCurve2Pt;
  double prevPlnX = NumericTraits<double>::min();
  double plnX;
  while (it.next(imgCurve1Pt, imgCurve2Pt, plnX)) {
    if (plnX <= prevPlnX) {
      // This means our surface has an S-like shape.
      // We don't support that, and to make ReverseArcLength happy,
      // we have to skip such points.
      continue;
    }

    const QLineF imgGeneratrix(imgCurve1Pt, imgCurve2Pt);
    const Vec2d imgLine1Pt(m_pln2img(Vec2d(plnX, 0)));
    const Vec2d imgLine2Pt(m_pln2img(Vec2d(plnX, 1)));

    const ToLineProjector projector(imgGeneratrix);
    const double y1 = projector.projectionScalar(imgLine1Pt);
    const double y2 = projector.projectionScalar(imgLine2Pt);

    double elevation = m_depthPerception * (1.0 - (y2 - y1));
    elevation = qBound(-0.5, elevation, 0.5);

    m_arcLengthMapper.addSample(plnX, elevation);
    prevElevation = elevation;
    prevPlnX = plnX;
  }

  // Needs to go before normalizeRange().
  m_directrixArcLength = m_arcLengthMapper.totalArcLength();
  // Scale arc lengths to the range of [0, 1].
  m_arcLengthMapper.normalizeRange(1);
}  // CylindricalSurfaceDewarper::initArcLengthMapper

/*======================= CoupledPolylinesIterator =========================*/

CylindricalSurfaceDewarper::CoupledPolylinesIterator::CoupledPolylinesIterator(
    const std::vector<QPointF>& imgDirectrix1,
    const std::vector<QPointF>& imgDirectrix2,
    const HomographicTransform<2, double>& pln2img,
    const HomographicTransform<2, double>& img2pln)
    : m_seq1It(imgDirectrix1.begin()),
      m_seq2It(imgDirectrix2.begin()),
      m_seq1End(imgDirectrix1.end()),
      m_seq2End(imgDirectrix2.end()),
      m_pln2img(pln2img),
      m_img2pln(img2pln),
      m_prevImgPt1(*m_seq1It),
      m_prevImgPt2(*m_seq2It),
      m_nextImgPt1(m_prevImgPt1),
      m_nextImgPt2(m_prevImgPt2),
      m_nextPlnX1(0),
      m_nextPlnX2(0) {}

bool CylindricalSurfaceDewarper::CoupledPolylinesIterator::next(QPointF& imgPt1, QPointF& imgPt2, double& plnX) {
  if ((m_nextPlnX1 < m_nextPlnX2) && (m_seq1It != m_seq1End)) {
    next1(imgPt1, imgPt2, plnX);
    return true;
  } else if (m_seq2It != m_seq2End) {
    next2(imgPt1, imgPt2, plnX);
    return true;
  } else {
    return false;
  }
}

void CylindricalSurfaceDewarper::CoupledPolylinesIterator::next1(QPointF& imgPt1, QPointF& imgPt2, double& plnX) {
  const Vec2d plnPt1(m_img2pln(m_nextImgPt1));
  plnX = plnPt1[0];
  imgPt1 = m_nextImgPt1;

  const Vec2d plnPtx(plnPt1[0], plnPt1[1] + 1);
  const Vec2d imgPtx(m_pln2img(plnPtx));
#if QT_VERSION_MAJOR == 5 && QT_VERSION_MINOR < 14
  auto intersect = QLineF(imgPt1, imgPtx).intersect(QLineF(m_nextImgPt2, m_prevImgPt2), &imgPt2);
#else
  auto intersect = QLineF(imgPt1, imgPtx).intersects(QLineF(m_nextImgPt2, m_prevImgPt2), &imgPt2);
#endif
  if (intersect == QLineF::NoIntersection) {
    imgPt2 = m_nextImgPt2;
  }

  advance1();
  if ((m_seq2It != m_seq2End) && (Vec2d(m_nextImgPt2 - imgPt2).squaredNorm() < 1)) {
    advance2();
  }
}

void CylindricalSurfaceDewarper::CoupledPolylinesIterator::next2(QPointF& imgPt1, QPointF& imgPt2, double& plnX) {
  const Vec2d plnPt2(m_img2pln(m_nextImgPt2));
  plnX = plnPt2[0];
  imgPt2 = m_nextImgPt2;

  const Vec2d plnPtx(plnPt2[0], plnPt2[1] + 1);
  const Vec2d imgPtx(m_pln2img(plnPtx));

#if QT_VERSION_MAJOR == 5 && QT_VERSION_MINOR < 14
  auto intersect = QLineF(imgPt2, imgPtx).intersect(QLineF(m_nextImgPt1, m_prevImgPt1), &imgPt1);
#else
  auto intersect = QLineF(imgPt2, imgPtx).intersects(QLineF(m_nextImgPt1, m_prevImgPt1), &imgPt1);
#endif
  if (intersect == QLineF::NoIntersection) {
    imgPt1 = m_nextImgPt1;
  }

  advance2();
  if ((m_seq1It != m_seq1End) && (Vec2d(m_nextImgPt1 - imgPt1).squaredNorm() < 1)) {
    advance1();
  }
}

void CylindricalSurfaceDewarper::CoupledPolylinesIterator::advance1() {
  if (++m_seq1It == m_seq1End) {
    return;
  }

  m_prevImgPt1 = m_nextImgPt1;
  m_nextImgPt1 = *m_seq1It;
  m_nextPlnX1 = m_img2pln(m_nextImgPt1)[0];
}

void CylindricalSurfaceDewarper::CoupledPolylinesIterator::advance2() {
  if (++m_seq2It == m_seq2End) {
    return;
  }

  m_prevImgPt2 = m_nextImgPt2;
  m_nextImgPt2 = *m_seq2It;
  m_nextPlnX2 = m_img2pln(m_nextImgPt2)[0];
}
}  // namespace dewarping
