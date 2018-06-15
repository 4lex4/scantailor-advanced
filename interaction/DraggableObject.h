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

#ifndef DRAGGABLE_OBJECT_H_
#define DRAGGABLE_OBJECT_H_

#include <boost/function.hpp>
#include "InteractionState.h"
#include "Proximity.h"

class ObjectDragHandler;
class QPoint;
class QPointF;
class QPainter;

class DraggableObject {
 public:
  typedef boost::function<void(QPainter& painter, const InteractionState& interaction)> PaintCallback;

  typedef boost::function<Proximity(const InteractionState& interaction)> ProximityThresholdCallback;

  typedef boost::function<int()> ProximityPriorityCallback;

  typedef boost::function<Proximity(const QPointF& mouse_pos)> ProximityCallback;

  typedef boost::function<void(const QPointF& mouse_pos)> DragInitiatedCallback;

  typedef boost::function<void(const QPointF& mouse_pos, Qt::KeyboardModifiers mask)> DragContinuationCallback;

  typedef boost::function<void(const QPointF& mouse_pos)> DragFinishedCallback;

  DraggableObject()
      : m_paintCallback(&DraggableObject::defaultPaint),
        m_proximityThresholdCallback(&DraggableObject::defaultProximityThreshold),
        m_proximityPriorityCallback(&DraggableObject::defaultProximityPriority),
        m_proximityCallback(),
        m_dragInitiatedCallback(),
        m_dragContinuationCallback(),
        m_dragFinishedCallback(&DraggableObject::defaultDragFinished) {}

  virtual ~DraggableObject() = default;

  virtual void paint(QPainter& painter, const InteractionState& interaction) { m_paintCallback(painter, interaction); }

  void setPaintCallback(const PaintCallback& callback) { m_paintCallback = callback; }

  /**
   * \return The maximum distance from the object (in widget coordinates) that
   *         still allows to initiate a dragging operation.
   */
  virtual Proximity proximityThreshold(const InteractionState& interaction) const {
    return m_proximityThresholdCallback(interaction);
  }

  void setProximityThresholdCallback(const ProximityThresholdCallback& callback) {
    m_proximityThresholdCallback = callback;
  }

  /**
   * Sometimes a more distant object should be selected for dragging in favor of
   * a closer one.  Consider for example a line segment with handles at its endpoints.
   * In this example, you would assign higher priority to those handles.
   */
  virtual int proximityPriority() const { return m_proximityPriorityCallback(); }

  void setProximityPriorityCallback(const ProximityPriorityCallback& callback) {
    m_proximityPriorityCallback = callback;
  }

  /**
   * \return The proximity from the mouse position in widget coordinates to
   *         any draggable part of the object.
   */
  virtual Proximity proximity(const QPointF& widget_mouse_pos) { return m_proximityCallback(widget_mouse_pos); }

  void setProximityCallback(const ProximityCallback& callback) { m_proximityCallback = callback; }

  /**
   * \brief Called when dragging is initiated, that is when the mouse button is pressed.
   */
  virtual void dragInitiated(const QPointF& mouse_pos) { m_dragInitiatedCallback(mouse_pos); }

  void setDragInitiatedCallback(const DragInitiatedCallback& callback) { m_dragInitiatedCallback = callback; }

  /**
   * \brief Handles a request to move to a particular position in widget coordinates.
   */
  virtual void dragContinuation(const QPointF& mouse_pos, Qt::KeyboardModifiers mask) {
    m_dragContinuationCallback(mouse_pos, mask);
  }

  void setDragContinuationCallback(const DragContinuationCallback& callback) { m_dragContinuationCallback = callback; }

  /**
   * \brief Called when dragging is finished, that is when the mouse button is released.
   */
  virtual void dragFinished(const QPointF& mouse_pos) { m_dragFinishedCallback(mouse_pos); }

  void setDragFinishedCallback(const DragFinishedCallback& callback) { m_dragFinishedCallback = callback; }

 private:
  static void defaultPaint(QPainter&, const InteractionState&) {}

  static Proximity defaultProximityThreshold(const InteractionState& interaction) {
    return interaction.proximityThreshold();
  }

  static int defaultProximityPriority() { return 0; }

  static void defaultDragFinished(const QPointF&) {}

  PaintCallback m_paintCallback;
  ProximityThresholdCallback m_proximityThresholdCallback;
  ProximityPriorityCallback m_proximityPriorityCallback;
  ProximityCallback m_proximityCallback;
  DragInitiatedCallback m_dragInitiatedCallback;
  DragContinuationCallback m_dragContinuationCallback;
  DragFinishedCallback m_dragFinishedCallback;
};


#endif  // ifndef DRAGGABLE_OBJECT_H_
