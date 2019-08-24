// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "DraggablePoint.h"

DraggablePoint::DraggablePoint() : m_hitAreaRadius(), m_proximityPriority(1) {}

Proximity DraggablePoint::proximityThreshold(const InteractionState& state) const {
  if (m_hitAreaRadius == 0.0) {
    return state.proximityThreshold();
  } else {
    return Proximity::fromDist(m_hitAreaRadius);
  }
}

int DraggablePoint::proximityPriority() const {
  return m_proximityPriority;
}

Proximity DraggablePoint::proximity(const QPointF& mousePos) {
  return Proximity(pointPosition(), mousePos);
}

void DraggablePoint::dragInitiated(const QPointF& mousePos) {
  m_pointRelativeToMouse = pointPosition() - mousePos;
}

void DraggablePoint::dragContinuation(const QPointF& mousePos, Qt::KeyboardModifiers mask) {
  pointMoveRequest(mousePos + m_pointRelativeToMouse, mask);
}
