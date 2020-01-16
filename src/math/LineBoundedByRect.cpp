// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "LineBoundedByRect.h"

#include "LineIntersectionScalar.h"
#include "NumericTraits.h"

bool lineBoundedByRect(QLineF& line, const QRectF& rect) {
  const QLineF rect_lines[4] = {QLineF(rect.topLeft(), rect.topRight()), QLineF(rect.bottomLeft(), rect.bottomRight()),
                                QLineF(rect.topLeft(), rect.bottomLeft()), QLineF(rect.topRight(), rect.bottomRight())};

  double max = NumericTraits<double>::min();
  double min = NumericTraits<double>::max();

  double s1 = 0;
  double s2 = 0;
  for (const QLineF& rectLine : rect_lines) {
    if (!lineIntersectionScalar(rectLine, line, s1, s2)) {
      // line is parallel to rectLine.
      continue;
    }
    if ((s1 < 0) || (s1 > 1)) {
      // Intersection outside of rect.
      continue;
    }

    if (s2 > max) {
      max = s2;
    }
    if (s2 < min) {
      min = s2;
    }
  }

  if (max > min) {
    line = QLineF(line.pointAt(min), line.pointAt(max));
    return true;
  } else {
    return false;
  }
}  // lineBoundedByRect
