// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_INTERACTION_DRAGGABLEOBJECT_H_
#define SCANTAILOR_INTERACTION_DRAGGABLEOBJECT_H_

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

  typedef boost::function<Proximity(const QPointF& mousePos)> ProximityCallback;

  typedef boost::function<void(const QPointF& mousePos)> DragInitiatedCallback;

  typedef boost::function<void(const QPointF& mousePos, Qt::KeyboardModifiers mask)> DragContinuationCallback;

  typedef boost::function<void(const QPointF& mousePos)> DragFinishedCallback;

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
  virtual Proximity proximity(const QPointF& widgetMousePos) { return m_proximityCallback(widgetMousePos); }

  void setProximityCallback(const ProximityCallback& callback) { m_proximityCallback = callback; }

  /**
   * \brief Called when dragging is initiated, that is when the mouse button is pressed.
   */
  virtual void dragInitiated(const QPointF& mousePos) { m_dragInitiatedCallback(mousePos); }

  void setDragInitiatedCallback(const DragInitiatedCallback& callback) { m_dragInitiatedCallback = callback; }

  /**
   * \brief Handles a request to move to a particular position in widget coordinates.
   */
  virtual void dragContinuation(const QPointF& mousePos, Qt::KeyboardModifiers mask) {
    m_dragContinuationCallback(mousePos, mask);
  }

  void setDragContinuationCallback(const DragContinuationCallback& callback) { m_dragContinuationCallback = callback; }

  /**
   * \brief Called when dragging is finished, that is when the mouse button is released.
   */
  virtual void dragFinished(const QPointF& mousePos) { m_dragFinishedCallback(mousePos); }

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


#endif  // ifndef SCANTAILOR_INTERACTION_DRAGGABLEOBJECT_H_
