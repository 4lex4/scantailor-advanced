// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "RastLineFinder.h"
#include <boost/foreach.hpp>
#include <cassert>
#include <cmath>
#include "Constants.h"
#include "VecNT.h"

namespace imageproc {
/*========================= RastLineFinderParams ===========================*/

RastLineFinderParams::RastLineFinderParams()
    : m_origin(0, 0),
      m_minAngleDeg(0),
      m_maxAngleDeg(180),
      m_angleToleranceDeg(0.1),
      m_maxDistFromLine(1.0),
      m_minSupportPoints(3) {}

bool RastLineFinderParams::validate(std::string* error) const {
  if (m_angleToleranceDeg <= 0) {
    if (error) {
      *error = "RastLineFinder: angle tolerance must be positive";
    }

    return false;
  }

  if (m_angleToleranceDeg >= 180) {
    if (error) {
      *error = "RastLineFinder: angle tolerance must be below 180 degrees";
    }

    return false;
  }

  if (m_maxDistFromLine <= 0) {
    if (error) {
      *error = "RastLineFinder: max-dist-from-line must be positive";
    }

    return false;
  }

  if (m_minSupportPoints < 2) {
    if (error) {
      *error = "RastLineFinder: min-support-points must be at least 2";
    }

    return false;
  }

  return true;
}  // RastLineFinderParams::validate

RastLineFinder::RastLineFinder(const std::vector<QPointF>& points, const RastLineFinderParams& params)
    : m_origin(params.origin()),
      m_angleToleranceRad(params.angleToleranceDeg() * constants::DEG2RAD),
      m_maxDistFromLine(params.maxDistFromLine()),
      m_minSupportPoints(params.minSupportPoints()),
      m_firstLine(true) {
  std::string error;
  if (!params.validate(&error)) {
    throw std::invalid_argument(error);
  }

  m_points.reserve(points.size());
  std::vector<unsigned> candidateIdxs;
  candidateIdxs.reserve(points.size());

  double maxSqdist = 0;

  for (const QPointF& pt : points) {
    m_points.emplace_back(pt);
    candidateIdxs.push_back(static_cast<unsigned int&&>(candidateIdxs.size()));

    const double sqdist = Vec2d(pt - m_origin).squaredNorm();
    if (sqdist > maxSqdist) {
      maxSqdist = sqdist;
    }
  }

  const auto maxDist = static_cast<float>(std::sqrt(maxSqdist) + 1.0);  // + 1.0 to combant rounding issues

  double deltaDeg = std::fmod(params.maxAngleDeg() - params.minAngleDeg(), 360.0);
  if (deltaDeg < 0) {
    deltaDeg += 360;
  }
  const double minAngleDeg = std::fmod(params.minAngleDeg(), 360.0);
  const double maxAngleDeg = minAngleDeg + deltaDeg;

  SearchSpace ssp(*this, -maxDist, maxDist, static_cast<float>(minAngleDeg * constants::DEG2RAD),
                  static_cast<float>(maxAngleDeg * constants::DEG2RAD), candidateIdxs);
  if (ssp.pointIdxs().size() >= m_minSupportPoints) {
    m_orderedSearchSpaces.pushDestructive(ssp);
  }
}

QLineF RastLineFinder::findNext(std::vector<unsigned>* pointIdxs) {
  if (m_firstLine) {
    m_firstLine = false;
  } else {
    pruneUnavailablePoints();
  }

  SearchSpace distSsp1, dist_ssp2;
  SearchSpace angleSsp1, angle_ssp2;

  while (!m_orderedSearchSpaces.empty()) {
    SearchSpace ssp;
    m_orderedSearchSpaces.retrieveFront(ssp);

    if (!ssp.subdivideDist(*this, distSsp1, dist_ssp2)) {
      if (!ssp.subdivideAngle(*this, angleSsp1, angle_ssp2)) {
        // Can't subdivide at all - return what we've got then.
        markPointsUnavailable(ssp.pointIdxs());
        if (pointIdxs) {
          pointIdxs->swap(ssp.pointIdxs());
        }

        return ssp.representativeLine(*this);
      } else {
        // Can only subdivide by angle.
        pushIfGoodEnough(angleSsp1);
        pushIfGoodEnough(angle_ssp2);
      }
    } else {
      if (!ssp.subdivideAngle(*this, angleSsp1, angle_ssp2)) {
        // Can only subdivide by distance.
        pushIfGoodEnough(distSsp1);
        pushIfGoodEnough(dist_ssp2);
      } else {
        // Can subdivide both by angle and distance.
        // Choose the option that results in less combined
        // number of points in two resulting sub-spaces.
        if (distSsp1.pointIdxs().size() + dist_ssp2.pointIdxs().size()
            < angleSsp1.pointIdxs().size() + angle_ssp2.pointIdxs().size()) {
          pushIfGoodEnough(distSsp1);
          pushIfGoodEnough(dist_ssp2);
        } else {
          pushIfGoodEnough(angleSsp1);
          pushIfGoodEnough(angle_ssp2);
        }
      }
    }
  }

  return QLineF();
}  // RastLineFinder::findNext

void RastLineFinder::pushIfGoodEnough(SearchSpace& ssp) {
  if (ssp.pointIdxs().size() >= m_minSupportPoints) {
    m_orderedSearchSpaces.pushDestructive(ssp);
  }
}

void RastLineFinder::markPointsUnavailable(const std::vector<unsigned>& pointIdxs) {
  for (unsigned idx : pointIdxs) {
    m_points[idx].available = false;
  }
}

void RastLineFinder::pruneUnavailablePoints() {
  OrderedSearchSpaces newSearchSpaces;
  SearchSpace ssp;
  PointUnavailablePred pred(&m_points);

  while (!m_orderedSearchSpaces.empty()) {
    m_orderedSearchSpaces.retrieveFront(ssp);
    ssp.pruneUnavailablePoints(pred);
    if (ssp.pointIdxs().size() >= m_minSupportPoints) {
      newSearchSpaces.pushDestructive(ssp);
    }
  }

  m_orderedSearchSpaces.swapWith(newSearchSpaces);
}

/*============================= SearchSpace ================================*/

RastLineFinder::SearchSpace::SearchSpace() : m_minDist(0), m_maxDist(0), m_minAngleRad(0), m_maxAngleRad(0) {}

RastLineFinder::SearchSpace::SearchSpace(const RastLineFinder& owner,
                                         float minDist,
                                         float maxDist,
                                         float minAngleRad,
                                         float maxAngleRad,
                                         const std::vector<unsigned>& candidateIdxs)
    : m_minDist(minDist), m_maxDist(maxDist), m_minAngleRad(minAngleRad), m_maxAngleRad(maxAngleRad) {
  m_pointIdxs.reserve(candidateIdxs.size());

  const QPointF origin(owner.m_origin);

  const double minSqdist = double(m_minDist) * double(m_minDist);
  const double maxSqdist = double(m_maxDist) * double(m_maxDist);

  const QPointF minAngleUnitVec(std::cos(m_minAngleRad), std::sin(m_minAngleRad));
  const QPointF maxAngleUnitVec(std::cos(m_maxAngleRad), std::sin(m_maxAngleRad));

  const QPointF minAngleInnerPt(origin + minAngleUnitVec * m_minDist);
  const QPointF maxAngleInnerPt(origin + maxAngleUnitVec * m_minDist);

  const QPointF minAngleOuterPt(origin + minAngleUnitVec * m_maxDist);
  const QPointF maxAngleOuterPt(origin + maxAngleUnitVec * m_maxDist);

  const Vec2d minTowardsMaxAngleVec(-minAngleUnitVec.y(), minAngleUnitVec.x());
  const Vec2d maxTowardsMinAngleVec(maxAngleUnitVec.y(), -maxAngleUnitVec.x());

  for (unsigned idx : candidateIdxs) {
    const Point& pnt = owner.m_points[idx];
    if (!pnt.available) {
      continue;
    }

    const Vec2d relPt(pnt.pt - origin);

    if ((Vec2d(pnt.pt - minAngleInnerPt).dot(minAngleUnitVec) >= 0)
        && (Vec2d(pnt.pt - maxAngleOuterPt).dot(maxAngleUnitVec) <= 0)) {
      // Accepted.
    } else if ((Vec2d(pnt.pt - maxAngleInnerPt).dot(maxAngleUnitVec) >= 0)
               && (Vec2d(pnt.pt - minAngleOuterPt).dot(minAngleUnitVec) <= 0)) {
      // Accepted.
    } else if ((minTowardsMaxAngleVec.dot(relPt) >= 0) && (maxTowardsMinAngleVec.dot(relPt) >= 0)
               && (relPt.squaredNorm() >= minSqdist) && (relPt.squaredNorm() <= maxSqdist)) {
      // Accepted.
    } else {
      // Rejected.
      continue;
    }

    m_pointIdxs.push_back(idx);
  }

  // Compact m_pointIdxs, as we expect a lot of SearchSpace objects
  // to exist at the same time.
  m_pointIdxs.shrink_to_fit();
}

QLineF RastLineFinder::SearchSpace::representativeLine(const RastLineFinder& owner) const {
  const float dist = 0.5f * (m_minDist + m_maxDist);
  const float angle = 0.5f * (m_minAngleRad + m_maxAngleRad);
  const QPointF angleUnitVec(std::cos(angle), std::sin(angle));
  const QPointF angleNormVec(-angleUnitVec.y(), angleUnitVec.x());
  const QPointF p1(owner.m_origin + angleUnitVec * dist);
  const QPointF p2(p1 + angleNormVec);

  return QLineF(p1, p2);
}

bool RastLineFinder::SearchSpace::subdivideDist(const RastLineFinder& owner,
                                                SearchSpace& subspace1,
                                                SearchSpace& subspace2) const {
  assert(m_maxDist >= m_minDist);

  if ((m_maxDist - m_minDist <= owner.m_maxDistFromLine * 2.0001) || (m_pointIdxs.size() < 2)) {
    return false;
  }

  if (m_maxDist - m_minDist <= owner.m_angleToleranceRad * 3) {
    // This branch prevents near-infinite subdivision that would have happened without it.
    SearchSpace ssp1(owner, m_minDist, static_cast<float>(m_minDist + owner.m_maxDistFromLine * 2), m_minAngleRad,
                     m_maxAngleRad, m_pointIdxs);
    SearchSpace ssp2(owner, static_cast<float>(m_maxDist - owner.m_maxDistFromLine * 2), m_maxDist, m_minAngleRad,
                     m_maxAngleRad, m_pointIdxs);
    ssp1.swap(subspace1);
    ssp2.swap(subspace2);
  } else {
    const float midDist = 0.5f * (m_maxDist + m_minDist);
    SearchSpace ssp1(owner, m_minDist, static_cast<float>(midDist + owner.m_maxDistFromLine), m_minAngleRad,
                     m_maxAngleRad, m_pointIdxs);
    SearchSpace ssp2(owner, static_cast<float>(midDist - owner.m_maxDistFromLine), m_maxDist, m_minAngleRad,
                     m_maxAngleRad, m_pointIdxs);
    ssp1.swap(subspace1);
    ssp2.swap(subspace2);
  }

  return true;
}

bool RastLineFinder::SearchSpace::subdivideAngle(const RastLineFinder& owner,
                                                 SearchSpace& subspace1,
                                                 SearchSpace& subspace2) const {
  assert(m_maxAngleRad >= m_minAngleRad);

  if ((m_maxAngleRad - m_minAngleRad <= owner.m_angleToleranceRad * 2) || (m_pointIdxs.size() < 2)) {
    return false;
  }

  const float midAngleRad = 0.5f * (m_maxAngleRad + m_minAngleRad);

  SearchSpace ssp1(owner, m_minDist, m_maxDist, m_minAngleRad, midAngleRad, m_pointIdxs);
  SearchSpace ssp2(owner, m_minDist, m_maxDist, midAngleRad, m_maxAngleRad, m_pointIdxs);

  ssp1.swap(subspace1);
  ssp2.swap(subspace2);

  return true;
}

void RastLineFinder::SearchSpace::pruneUnavailablePoints(PointUnavailablePred pred) {
  m_pointIdxs.resize(std::remove_if(m_pointIdxs.begin(), m_pointIdxs.end(), pred) - m_pointIdxs.begin());
}

void RastLineFinder::SearchSpace::swap(SearchSpace& other) {
  std::swap(m_minDist, other.m_minDist);
  std::swap(m_maxDist, other.m_maxDist);
  std::swap(m_minAngleRad, other.m_minAngleRad);
  std::swap(m_maxAngleRad, other.m_maxAngleRad);
  m_pointIdxs.swap(other.m_pointIdxs);
}
}  // namespace imageproc
