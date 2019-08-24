// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_ZONES_SPLINESEGMENT_H_
#define SCANTAILOR_ZONES_SPLINESEGMENT_H_

#include <QLineF>
#include <QPointF>
#include "SplineVertex.h"

class SplineSegment {
 public:
  SplineVertex::Ptr prev;
  SplineVertex::Ptr next;

  SplineSegment() = default;

  SplineSegment(const SplineVertex::Ptr& prev, const SplineVertex::Ptr& next);

  SplineVertex::Ptr splitAt(const QPointF& pt);

  bool isValid() const;

  bool operator==(const SplineSegment& other) const { return prev == other.prev && next == other.next; }

  QLineF toLine() const { return QLineF(prev->point(), next->point()); }
};


#endif  // ifndef SCANTAILOR_ZONES_SPLINESEGMENT_H_
