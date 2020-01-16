// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "SplineSegment.h"

#include <cassert>

SplineSegment::SplineSegment(const SplineVertex::Ptr& prev, const SplineVertex::Ptr& next) : prev(prev), next(next) {}

SplineVertex::Ptr SplineSegment::splitAt(const QPointF& pt) {
  assert(isValid());
  return prev->insertAfter(pt);
}

bool SplineSegment::isValid() const {
  return prev && next && prev->next(SplineVertex::LOOP_IF_BRIDGED) == next;
}
