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
  std::vector<unsigned> candidate_idxs;
  candidate_idxs.reserve(points.size());

  double max_sqdist = 0;

  for (const QPointF& pt : points) {
    m_points.emplace_back(pt);
    candidate_idxs.push_back(static_cast<unsigned int&&>(candidate_idxs.size()));

    const double sqdist = Vec2d(pt - m_origin).squaredNorm();
    if (sqdist > max_sqdist) {
      max_sqdist = sqdist;
    }
  }

  const auto max_dist = static_cast<const float>(std::sqrt(max_sqdist) + 1.0);  // + 1.0 to combant rounding issues

  double delta_deg = std::fmod(params.maxAngleDeg() - params.minAngleDeg(), 360.0);
  if (delta_deg < 0) {
    delta_deg += 360;
  }
  const double min_angle_deg = std::fmod(params.minAngleDeg(), 360.0);
  const double max_angle_deg = min_angle_deg + delta_deg;

  SearchSpace ssp(*this, -max_dist, max_dist, static_cast<float>(min_angle_deg * constants::DEG2RAD),
                  static_cast<float>(max_angle_deg * constants::DEG2RAD), candidate_idxs);
  if (ssp.pointIdxs().size() >= m_minSupportPoints) {
    m_orderedSearchSpaces.pushDestructive(ssp);
  }
}

QLineF RastLineFinder::findNext(std::vector<unsigned>* point_idxs) {
  if (m_firstLine) {
    m_firstLine = false;
  } else {
    pruneUnavailablePoints();
  }

  SearchSpace dist_ssp1, dist_ssp2;
  SearchSpace angle_ssp1, angle_ssp2;

  while (!m_orderedSearchSpaces.empty()) {
    SearchSpace ssp;
    m_orderedSearchSpaces.retrieveFront(ssp);

    if (!ssp.subdivideDist(*this, dist_ssp1, dist_ssp2)) {
      if (!ssp.subdivideAngle(*this, angle_ssp1, angle_ssp2)) {
        // Can't subdivide at all - return what we've got then.
        markPointsUnavailable(ssp.pointIdxs());
        if (point_idxs) {
          point_idxs->swap(ssp.pointIdxs());
        }

        return ssp.representativeLine(*this);
      } else {
        // Can only subdivide by angle.
        pushIfGoodEnough(angle_ssp1);
        pushIfGoodEnough(angle_ssp2);
      }
    } else {
      if (!ssp.subdivideAngle(*this, angle_ssp1, angle_ssp2)) {
        // Can only subdivide by distance.
        pushIfGoodEnough(dist_ssp1);
        pushIfGoodEnough(dist_ssp2);
      } else {
        // Can subdivide both by angle and distance.
        // Choose the option that results in less combined
        // number of points in two resulting sub-spaces.
        if (dist_ssp1.pointIdxs().size() + dist_ssp2.pointIdxs().size()
            < angle_ssp1.pointIdxs().size() + angle_ssp2.pointIdxs().size()) {
          pushIfGoodEnough(dist_ssp1);
          pushIfGoodEnough(dist_ssp2);
        } else {
          pushIfGoodEnough(angle_ssp1);
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

void RastLineFinder::markPointsUnavailable(const std::vector<unsigned>& point_idxs) {
  for (unsigned idx : point_idxs) {
    m_points[idx].available = false;
  }
}

void RastLineFinder::pruneUnavailablePoints() {
  OrderedSearchSpaces new_search_spaces;
  SearchSpace ssp;
  PointUnavailablePred pred(&m_points);

  while (!m_orderedSearchSpaces.empty()) {
    m_orderedSearchSpaces.retrieveFront(ssp);
    ssp.pruneUnavailablePoints(pred);
    if (ssp.pointIdxs().size() >= m_minSupportPoints) {
      new_search_spaces.pushDestructive(ssp);
    }
  }

  m_orderedSearchSpaces.swapWith(new_search_spaces);
}

/*============================= SearchSpace ================================*/

RastLineFinder::SearchSpace::SearchSpace() : m_minDist(0), m_maxDist(0), m_minAngleRad(0), m_maxAngleRad(0) {}

RastLineFinder::SearchSpace::SearchSpace(const RastLineFinder& owner,
                                         float min_dist,
                                         float max_dist,
                                         float min_angle_rad,
                                         float max_angle_rad,
                                         const std::vector<unsigned>& candidate_idxs)
    : m_minDist(min_dist), m_maxDist(max_dist), m_minAngleRad(min_angle_rad), m_maxAngleRad(max_angle_rad) {
  m_pointIdxs.reserve(candidate_idxs.size());

  const QPointF origin(owner.m_origin);

  const double min_sqdist = double(m_minDist) * double(m_minDist);
  const double max_sqdist = double(m_maxDist) * double(m_maxDist);

  const QPointF min_angle_unit_vec(std::cos(m_minAngleRad), std::sin(m_minAngleRad));
  const QPointF max_angle_unit_vec(std::cos(m_maxAngleRad), std::sin(m_maxAngleRad));

  const QPointF min_angle_inner_pt(origin + min_angle_unit_vec * m_minDist);
  const QPointF max_angle_inner_pt(origin + max_angle_unit_vec * m_minDist);

  const QPointF min_angle_outer_pt(origin + min_angle_unit_vec * m_maxDist);
  const QPointF max_angle_outer_pt(origin + max_angle_unit_vec * m_maxDist);

  const Vec2d min_towards_max_angle_vec(-min_angle_unit_vec.y(), min_angle_unit_vec.x());
  const Vec2d max_towards_min_angle_vec(max_angle_unit_vec.y(), -max_angle_unit_vec.x());

  for (unsigned idx : candidate_idxs) {
    const Point& pnt = owner.m_points[idx];
    if (!pnt.available) {
      continue;
    }

    const Vec2d rel_pt(pnt.pt - origin);

    if ((Vec2d(pnt.pt - min_angle_inner_pt).dot(min_angle_unit_vec) >= 0)
        && (Vec2d(pnt.pt - max_angle_outer_pt).dot(max_angle_unit_vec) <= 0)) {
      // Accepted.
    } else if ((Vec2d(pnt.pt - max_angle_inner_pt).dot(max_angle_unit_vec) >= 0)
               && (Vec2d(pnt.pt - min_angle_outer_pt).dot(min_angle_unit_vec) <= 0)) {
      // Accepted.
    } else if ((min_towards_max_angle_vec.dot(rel_pt) >= 0) && (max_towards_min_angle_vec.dot(rel_pt) >= 0)
               && (rel_pt.squaredNorm() >= min_sqdist) && (rel_pt.squaredNorm() <= max_sqdist)) {
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
  const QPointF angle_unit_vec(std::cos(angle), std::sin(angle));
  const QPointF angle_norm_vec(-angle_unit_vec.y(), angle_unit_vec.x());
  const QPointF p1(owner.m_origin + angle_unit_vec * dist);
  const QPointF p2(p1 + angle_norm_vec);

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
    const float mid_dist = 0.5f * (m_maxDist + m_minDist);
    SearchSpace ssp1(owner, m_minDist, static_cast<float>(mid_dist + owner.m_maxDistFromLine), m_minAngleRad,
                     m_maxAngleRad, m_pointIdxs);
    SearchSpace ssp2(owner, static_cast<float>(mid_dist - owner.m_maxDistFromLine), m_maxDist, m_minAngleRad,
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

  const float mid_angle_rad = 0.5f * (m_maxAngleRad + m_minAngleRad);

  SearchSpace ssp1(owner, m_minDist, m_maxDist, m_minAngleRad, mid_angle_rad, m_pointIdxs);
  SearchSpace ssp2(owner, m_minDist, m_maxDist, mid_angle_rad, m_maxAngleRad, m_pointIdxs);

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
