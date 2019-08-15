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

#ifndef POLYLINE_INTERSECTOR_H_
#define POLYLINE_INTERSECTOR_H_

#include <QLineF>
#include <QPointF>
#include <vector>
#include "VecNT.h"

class PolylineIntersector {
 public:
  class Hint {
    friend class PolylineIntersector;

   public:
    Hint();

   private:
    void update(int new_segment);

    int m_lastSegment;
    int m_direction;
  };


  explicit PolylineIntersector(const std::vector<QPointF>& polyline);

  QPointF intersect(const QLineF& line, Hint& hint) const;

 private:
  bool intersectsSegment(const QLineF& normal, int segment) const;

  bool intersectsSpan(const QLineF& normal, const QLineF& span) const;

  QPointF intersectWithSegment(const QLineF& line, int segment) const;

  bool tryIntersectingOutsideOfPolyline(const QLineF& line, QPointF& intersection, Hint& hint) const;

  std::vector<QPointF> m_polyline;
  int m_numSegments;
};


#endif  // ifndef POLYLINE_INTERSECTOR_H_
