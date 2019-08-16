// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "DraggablePolygon.h"
#include <QPainter>

DraggablePolygon::DraggablePolygon() : m_proximityPriority(0) {}

int DraggablePolygon::proximityPriority() const {
  return m_proximityPriority;
}

Proximity DraggablePolygon::proximity(const QPointF& mouse_pos) {
  double value = polygonPosition().containsPoint(mouse_pos, Qt::WindingFill) ? 0 : std::numeric_limits<double>::max();
  return Proximity::fromSqDist(value);
}

void DraggablePolygon::dragInitiated(const QPointF& mouse_pos) {
  m_initialMousePos = mouse_pos;
  m_initialPolygonPos = polygonPosition();
}

void DraggablePolygon::dragContinuation(const QPointF& mouse_pos, Qt::KeyboardModifiers mask) {
  polygonMoveRequest(m_initialPolygonPos.translated(mouse_pos - m_initialMousePos));
}
