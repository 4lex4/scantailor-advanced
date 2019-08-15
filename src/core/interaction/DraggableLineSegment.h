/*
   Scan Tailor - Interactive post-processing tool for scanned pages.
   Copyright (C) 2007-2009  Joseph Artsimovich <joseph_a@mail.ru>

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

#ifndef DRAGGABLE_LINE_SEGMENT_H_
#define DRAGGABLE_LINE_SEGMENT_H_

#include <QLineF>
#include <QPointF>
#include <boost/function.hpp>
#include "DraggableObject.h"

class ObjectDragHandler;

class DraggableLineSegment : public DraggableObject {
 public:
  typedef boost::function<QLineF()> PositionCallback;

  typedef boost::function<void(const QLineF& line, Qt::KeyboardModifiers mask)> MoveRequestCallback;

  DraggableLineSegment();

  void setProximityPriority(int priority) { m_proximityPriority = priority; }

  int proximityPriority() const override;

  Proximity proximity(const QPointF& mouse_pos) override;

  void dragInitiated(const QPointF& mouse_pos) override;

  void dragContinuation(const QPointF& mouse_pos, Qt::KeyboardModifiers mask) override;

  void setPositionCallback(const PositionCallback& callback) { m_positionCallback = callback; }

  void setMoveRequestCallback(const MoveRequestCallback& callback) { m_moveRequestCallback = callback; }

 protected:
  virtual QLineF lineSegmentPosition() const { return m_positionCallback(); }

  virtual void lineSegmentMoveRequest(const QLineF& line, Qt::KeyboardModifiers mask) {
    m_moveRequestCallback(line, mask);
  }

 private:
  PositionCallback m_positionCallback;
  MoveRequestCallback m_moveRequestCallback;
  QPointF m_initialMousePos;
  QLineF m_initialLinePos;
  int m_proximityPriority;
};


#endif  // ifndef DRAGGABLE_LINE_SEGMENT_H_
