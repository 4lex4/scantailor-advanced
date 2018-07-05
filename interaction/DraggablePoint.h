/*
   Scan Tailor - Interactive post-processing tool for scanned pages.
   Copyright (C)  Joseph Artsimovich <joseph.artsimovich@gmail.com>

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

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
