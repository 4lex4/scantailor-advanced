// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "DraggableLineSegment.h"
#include <QPainter>

DraggableLineSegment::DraggableLineSegment() : m_proximityPriority(0) {}

int DraggableLineSegment::proximityPriority() const {
  return m_proximityPriority;
}

Proximity DraggableLineSegment::proximity(const QPointF& mousePos) {
  return Proximity::pointAndLineSegment(mousePos, lineSegmentPosition());
}

void DraggableLineSegment::dragInitiated(const QPointF& mousePos) {
  m_initialMousePos = mousePos;
  m_initialLinePos = lineSegmentPosition();
}

void DraggableLineSegment::dragContinuation(const QPointF& mousePos, Qt::KeyboardModifiers mask) {
  lineSegmentMoveRequest(m_initialLinePos.translated(mousePos - m_initialMousePos), mask);
}
