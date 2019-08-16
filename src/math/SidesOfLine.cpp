// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "SidesOfLine.h"

qreal sidesOfLine(const QLineF& line, const QPointF& p1, const QPointF& p2) {
  const QPointF normal(line.normalVector().p2() - line.p1());
  const QPointF vec1(p1 - line.p1());
  const QPointF vec2(p2 - line.p1());
  const qreal dot1 = normal.x() * vec1.x() + normal.y() * vec1.y();
  const qreal dot2 = normal.x() * vec2.x() + normal.y() * vec2.y();

  return dot1 * dot2;
}
