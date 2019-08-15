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

#ifndef TO_LINE_PROJECTOR_H_
#define TO_LINE_PROJECTOR_H_

#include <QLineF>
#include <QPointF>
#include "VecNT.h"

/**
 * \brief Projects points onto a line (not a line segment).
 *
 * Projecting means finding the point on a line that is closest
 * to the given point.
 */
class ToLineProjector {
 public:
  /**
   * \brief Initializes line projector.
   *
   * If line endpoints match, all points will
   * be projected to that point.
   */
  explicit ToLineProjector(const QLineF& line);

  /**
   * \brief Finds the projection point.
   */
  QPointF projectionPoint(const QPointF& pt) const;

  /**
   * \brief Equivalent to projectionPoint(pt) - pt.
   */
  QPointF projectionVector(const QPointF& pt) const;

  /**
   * Solves the equation of:\n
   * line.p1() + x * (line.p2() - line.p1()) = p\n
   * for x, where p would be the projection point.
   * This function is faster than projectionPoint().
   */
  double projectionScalar(const QPointF& pt) const;

  /**
   * Returns the distance from \p pt to the projection point.
   */
  double projectionDist(const QPointF& pt) const;

  /**
   * Returns the squared distance from \p pt to the projection point.
   * This function is faster than projectionDist().
   */
  double projectionSqDist(const QPointF& pt) const;

 private:
  QPointF m_origin;
  QPointF m_vec;
  Vec2d m_mat;
};


#endif  // ifndef TO_LINE_PROJECTOR_H_
