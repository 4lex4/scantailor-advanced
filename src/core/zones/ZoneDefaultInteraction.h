// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef ZONE_DEFAULT_INTERACTION_H_
#define ZONE_DEFAULT_INTERACTION_H_

#include <QCoreApplication>
#include <QPointF>
#include "BasicSplineVisualizer.h"
#include "DragHandler.h"
#include "DragWatcher.h"
#include "EditableSpline.h"
#include "InteractionHandler.h"
#include "InteractionState.h"
#include "SplineSegment.h"
#include "SplineVertex.h"

class ZoneInteractionContext;

class ZoneDefaultInteraction : public InteractionHandler {
  Q_DECLARE_TR_FUNCTIONS(ZoneDefaultInteraction)
 public:
  explicit ZoneDefaultInteraction(ZoneInteractionContext& context);

 protected:
  ZoneInteractionContext& context() { return m_context; }

  void onPaint(QPainter& painter, const InteractionState& interaction) override;

  void onProximityUpdate(const QPointF& mouse_pos, InteractionState& interaction) override;

  void onMousePressEvent(QMouseEvent* event, InteractionState& interaction) override;

  void onMouseReleaseEvent(QMouseEvent* event, InteractionState& interaction) override;

  void onMouseMoveEvent(QMouseEvent* event, InteractionState& interaction) override;

  void onKeyPressEvent(QKeyEvent* event, InteractionState& interaction) override;

  void onKeyReleaseEvent(QKeyEvent* event, InteractionState& interaction) override;

  void onContextMenuEvent(QContextMenuEvent* event, InteractionState& interaction) override;

 private:
  ZoneInteractionContext& m_context;
  BasicSplineVisualizer m_visualizer;
  InteractionState::Captor m_vertexProximity;
  InteractionState::Captor m_segmentProximity;
  InteractionState::Captor m_zoneAreaProximity;
  InteractionState::Captor m_zoneAreaDragProximity;
  InteractionState::Captor m_zoneAreaDragCopyProximity;
  QPointF m_screenMousePos;
  Qt::KeyboardModifiers m_activeKeyboardModifiers;

  /**
   * We want our own drag handler, to be able to monitor it
   * and decide if we should go into zone creation state
   * after the left mouse button is released.
   */
  DragHandler m_dragHandler;

  /**
   * Because we hold an interaction state from constructor to destructor,
   * we have to have our own zoom handler with explicit interaction permission
   * if we want zoom to work.
   */
  DragWatcher m_dragWatcher;

  // These are valid if m_vertexProximity is the proximity leader.
  SplineVertex::Ptr m_nearestVertex;
  EditableSpline::Ptr m_nearestVertexSpline;

  // These are valid if m_segmentProximity is the proximity leader.
  SplineSegment m_nearestSegment;
  EditableSpline::Ptr m_nearestSegmentSpline;
  QPointF m_screenPointOnSegment;
  EditableSpline::Ptr m_underCursorSpline;
};


#endif  // ifndef ZONE_DEFAULT_INTERACTION_H_
