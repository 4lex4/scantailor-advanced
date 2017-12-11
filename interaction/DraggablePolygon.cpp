
#include "DraggablePolygon.h"
#include "ImageViewBase.h"
#include <QPainter>

DraggablePolygon::DraggablePolygon()
        : m_proximityPriority(0) {
}

int DraggablePolygon::proximityPriority() const {
    return m_proximityPriority;
}

Proximity DraggablePolygon::proximity(QPointF const& mouse_pos) {
    double value = polygonPosition().containsPoint(mouse_pos, Qt::WindingFill)
                   ? 0
                   : std::numeric_limits<double>::max();
    return Proximity::fromSqDist(value);
}

void DraggablePolygon::dragInitiated(QPointF const& mouse_pos) {
    m_initialMousePos = mouse_pos;
    m_initialPolygonPos = polygonPosition();
}

void DraggablePolygon::dragContinuation(QPointF const& mouse_pos, Qt::KeyboardModifiers mask) {
    polygonMoveRequest(m_initialPolygonPos.translated(mouse_pos - m_initialMousePos));
}

