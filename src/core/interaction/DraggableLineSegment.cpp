// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "DraggableLineSegment.h"
#include <QPainter>

DraggableLineSegment::DraggableLineSegment() : m_proximityPriority(0) {}

int DraggableLineSegment::proximityPriority() const {
  return m_proximityPriority;
}

Proximity DraggableLineSegment::proximity(const QPointF& mouse_pos) {
  return Proximity::pointAndLineSegment(mouse_pos, lineSegmentPosition());
}

void DraggableLineSegment::dragInitiated(const QPointF& mouse_pos) {
  m_initialMousePos = mouse_pos;
  m_initialLinePos = lineSegmentPosition();
}

void DraggableLineSegment::dragContinuation(const QPointF& mouse_pos, Qt::KeyboardModifiers mask) {
  lineSegmentMoveRequest(m_initialLinePos.translated(mouse_pos - m_initialMousePos), mask);
}
