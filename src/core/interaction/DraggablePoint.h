// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef DRAGGABLE_POINT_H_
#define DRAGGABLE_POINT_H_

#include <QPointF>
#include <boost/function.hpp>
#include "DraggableObject.h"

class DraggablePoint : public DraggableObject {
 public:
  typedef boost::function<QPointF()> PositionCallback;

  typedef boost::function<void(const QPointF& mouse_pos, Qt::KeyboardModifiers mask)> MoveRequestCallback;

  DraggablePoint();

  /**
   * Returns the hit area radius, with zero indicating the global
   * proximity threshold of InteractionState is to be used.
   */
  double hitRadius() const { return m_hitAreaRadius; }

  void setHitRadius(double radius) { m_hitAreaRadius = radius; }

  Proximity proximityThreshold(const InteractionState& interaction) const override;

  void setProximityPriority(int priority) { m_proximityPriority = priority; }

  int proximityPriority() const override;

  Proximity proximity(const QPointF& mouse_pos) override;

  void dragInitiated(const QPointF& mouse_pos) override;

  void dragContinuation(const QPointF& mouse_pos, Qt::KeyboardModifiers mask) override;

  void setPositionCallback(const PositionCallback& callback) { m_positionCallback = callback; }

  void setMoveRequestCallback(const MoveRequestCallback& callback) { m_moveRequestCallback = callback; }

 protected:
  virtual QPointF pointPosition() const { return m_positionCallback(); }

  virtual void pointMoveRequest(const QPointF& widget_pos, Qt::KeyboardModifiers mask) {
    m_moveRequestCallback(widget_pos, mask);
  }

 private:
  PositionCallback m_positionCallback;
  MoveRequestCallback m_moveRequestCallback;
  QPointF m_pointRelativeToMouse;
  double m_hitAreaRadius;
  int m_proximityPriority;
};


#endif  // ifndef DRAGGABLE_POINT_H_
