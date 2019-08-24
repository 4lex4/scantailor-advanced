// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_INTERACTION_DRAGGABLEPOLYGON_H_
#define SCANTAILOR_INTERACTION_DRAGGABLEPOLYGON_H_

#include <QPointF>
#include <QPolygonF>
#include <boost/function.hpp>
#include "DraggableObject.h"

class ObjectDragHandler;

class DraggablePolygon : public DraggableObject {
 public:
  using PositionCallback = boost::function<QPolygonF()>;

  using MoveRequestCallback = boost::function<void(const QPolygonF& polygon)>;

  DraggablePolygon();

  int proximityPriority() const override;

  Proximity proximity(const QPointF& mousePos) override;

  void dragInitiated(const QPointF& mousePos) override;

  void dragContinuation(const QPointF& mousePos, Qt::KeyboardModifiers mask) override;

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


#endif  // SCANTAILOR_INTERACTION_DRAGGABLEPOLYGON_H_
