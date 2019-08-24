// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "EditableSpline.h"
#include "SerializableSpline.h"

EditableSpline::EditableSpline() = default;

EditableSpline::EditableSpline(const SerializableSpline& spline) {
  for (const QPointF& pt : spline.toPolygon()) {
    appendVertex(pt);
  }

  SplineVertex::Ptr lastVertex(this->lastVertex());
  if (lastVertex && (firstVertex()->point() == lastVertex->point())) {
    lastVertex->remove();
  }

  setBridged(true);
}

void EditableSpline::appendVertex(const QPointF& pt) {
  m_sentinel.insertBefore(pt);
}

bool EditableSpline::hasAtLeastSegments(int num) const {
  for (SegmentIterator it((EditableSpline&) *this); num > 0 && it.hasNext(); it.next()) {
    --num;
  }

  return num == 0;
}

QPolygonF EditableSpline::toPolygon() const {
  QPolygonF poly;

  SplineVertex::Ptr vertex(firstVertex());
  for (; vertex; vertex = vertex->next(SplineVertex::NO_LOOP)) {
    poly.push_back(vertex->point());
  }

  vertex = lastVertex()->next(SplineVertex::LOOP_IF_BRIDGED);
  if (vertex) {
    poly.push_back(vertex->point());
  }

  return poly;
}

/*======================== Spline::SegmentIterator =======================*/

bool EditableSpline::SegmentIterator::hasNext() const {
  return m_nextVertex && m_nextVertex->next(SplineVertex::LOOP_IF_BRIDGED);
}

SplineSegment EditableSpline::SegmentIterator::next() {
  assert(hasNext());

  SplineVertex::Ptr origin(m_nextVertex);
  m_nextVertex = m_nextVertex->next(SplineVertex::NO_LOOP);
  if (!m_nextVertex) {
    return SplineSegment(origin, origin->next(SplineVertex::LOOP_IF_BRIDGED));
  } else {
    return SplineSegment(origin, m_nextVertex);
  }
}
