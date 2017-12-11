
#ifndef SCANTAILOR_DRAGGABLEPOLYGON_H
#define SCANTAILOR_DRAGGABLEPOLYGON_H

#include "DraggableObject.h"
#include <QPointF>
#include <QPolygonF>
#include <boost/function.hpp>

class ObjectDragHandler;

class DraggablePolygon : public DraggableObject {
public:
    typedef boost::function<
            QPolygonF()
    > PositionCallback;

    typedef boost::function<
            void(QPolygonF const& polygon)
    > MoveRequestCallback;

    DraggablePolygon();

    int proximityPriority() const override;

    Proximity proximity(QPointF const& mouse_pos) override;

    void dragInitiated(QPointF const& mouse_pos) override;

    void dragContinuation(QPointF const& mouse_pos, Qt::KeyboardModifiers mask) override;

    void setProximityPriority(int priority) {
        m_proximityPriority = priority;
    }

    void setPositionCallback(PositionCallback const& callback) {
        m_positionCallback = callback;
    }

    void setMoveRequestCallback(MoveRequestCallback const& callback) {
        m_moveRequestCallback = callback;
    }

protected:
    virtual QPolygonF polygonPosition() const {
        return m_positionCallback();
    }

    virtual void polygonMoveRequest(QPolygonF const& polygon) {
        m_moveRequestCallback(polygon);
    }

private:
    PositionCallback m_positionCallback;
    MoveRequestCallback m_moveRequestCallback;
    QPointF m_initialMousePos;
    QPolygonF m_initialPolygonPos;
    int m_proximityPriority;
};


#endif //SCANTAILOR_DRAGGABLEPOLYGON_H
