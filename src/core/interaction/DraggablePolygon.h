
#ifndef SCANTAILOR_DRAGGABLEPOLYGON_H
#define SCANTAILOR_DRAGGABLEPOLYGON_H

#include <QPointF>
#include <QPolygonF>
#include <boost/function.hpp>
#include "DraggableObject.h"

class ObjectDragHandler;

class DraggablePolygon : public DraggableObject {
 public:
  typedef boost::function<QPolygonF()> PositionCallback;

  typedef boost::function<void(const QPolygonF& polygon)> MoveRequestCallback;

  DraggablePolygon();

  int proximityPriority() const override;

  Proximity proximity(const QPointF& mouse_pos) override;

  void dragInitiated(const QPointF& mouse_pos) override;

  void dragContinuation(const QPointF& mouse_pos, Qt::KeyboardModifiers mask) override;

  void setProximityPriority(int priority) { m_proximityPriority = priority; }

  void setPositionCallback(const PositionCallback& callback) { m_positionCallback = callback; }

  void setMoveRequestCallback(const MoveRequestCallback& callback) { m_moveRequestCallback = callback; }

 protected:
  virtual QPolygonF polygonPosition() const { return m_positionCallback(); }

  virtual void polygonMoveRequest(const QPolygonF& polygon) { m_moveRequestCallback(polygon); }

 private:
  PositionCallback m_positionCallback;
  MoveRequestCallback m_moveRequestCallback;
  QPointF m_initialMousePos;
  QPolygonF m_initialPolygonPos;
  int m_proximityPriority;
};


#endif  // SCANTAILOR_DRAGGABLEPOLYGON_H
