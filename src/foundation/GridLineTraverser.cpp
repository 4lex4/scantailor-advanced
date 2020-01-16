// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "GridLineTraverser.h"

#include "LineIntersectionScalar.h"

GridLineTraverser::GridLineTraverser(const QLineF& line) {
  const QPoint p1(line.p1().toPoint());
  const QPoint p2(line.p2().toPoint());
  int hSpans, v_spans, num_spans;
  double s1 = 0.0, s2 = 0.0;
  if ((hSpans = std::abs(p1.x() - p2.x())) > (v_spans = std::abs(p1.y() - p2.y()))) {
    // Major direction: horizontal.
    num_spans = hSpans;
    lineIntersectionScalar(line, QLineF(p1, QPoint(p1.x(), p1.y() + 1)), s1);
    lineIntersectionScalar(line, QLineF(p2, QPoint(p2.x(), p2.y() + 1)), s2);
  } else {
    // Major direction: vertical.
    num_spans = v_spans;
    lineIntersectionScalar(line, QLineF(p1, QPoint(p1.x() + 1, p1.y())), s1);
    lineIntersectionScalar(line, QLineF(p2, QPoint(p2.x() + 1, p2.y())), s2);
  }

  m_dt = num_spans == 0 ? 0 : 1.0 / num_spans;
  m_line.setP1(line.pointAt(s1));
  m_line.setP2(line.pointAt(s2));
  m_totalStops = num_spans + 1;
  m_stopsDone = 0;
}

QPoint GridLineTraverser::next() {
  const QPointF pt(m_line.pointAt(m_stopsDone * m_dt));
  ++m_stopsDone;
  return pt.toPoint();
}
