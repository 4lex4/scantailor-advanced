// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_MATH_POLYLINEINTERSECTOR_H_
#define SCANTAILOR_MATH_POLYLINEINTERSECTOR_H_

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
    void update(int newSegment);

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


#endif  // ifndef SCANTAILOR_MATH_POLYLINEINTERSECTOR_H_
