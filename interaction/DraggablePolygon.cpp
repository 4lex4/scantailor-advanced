
#include "DraggablePolygon.h"
#include <QPainter>
#include "ImageViewBase.h"

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
