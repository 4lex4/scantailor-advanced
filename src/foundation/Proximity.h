// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef PROXIMITY_H_
#define PROXIMITY_H_

#include <cmath>
#include <limits>

class QPointF;
class QLineF;

class Proximity {
 public:
  Proximity() : m_sqDist(std::numeric_limits<double>::max()) {}

  Proximity(const QPointF& p1, const QPointF& p2);

  static Proximity fromDist(double dist) { return Proximity(dist * dist); }

  static Proximity fromSqDist(double sqDist) { return Proximity(sqDist); }

  static Proximity pointAndLineSegment(const QPointF& pt, const QLineF& segment, QPointF* point_on_segment = nullptr);

  double dist() const { return std::sqrt(m_sqDist); }

  double sqDist() const { return m_sqDist; }

  bool operator==(const Proximity& rhs) const { return m_sqDist == rhs.m_sqDist; }

  bool operator!=(const Proximity& rhs) const { return m_sqDist != rhs.m_sqDist; }

  bool operator<(const Proximity& rhs) const { return m_sqDist < rhs.m_sqDist; }

  bool operator>(const Proximity& rhs) const { return m_sqDist > rhs.m_sqDist; }

  bool operator<=(const Proximity& rhs) const { return m_sqDist <= rhs.m_sqDist; }

  bool operator>=(const Proximity& rhs) const { return m_sqDist >= rhs.m_sqDist; }

 private:
  explicit Proximity(double sqDist) : m_sqDist(sqDist) {}

  double m_sqDist;
};


#endif  // ifndef PROXIMITY_H_
