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
  ZoneInteractionContext& context() { return m_rContext; }

  void onPaint(QPainter& painter, const InteractionState& interaction) override;

  void onProximityUpdate(const QPointF& mouse_pos, InteractionState& interaction) override;

  void onMousePressEvent(QMouseEvent* event, InteractionState& interaction) override;

  void onMouseReleaseEvent(QMouseEvent* event, InteractionState& interaction) override;

  void onMouseMoveEvent(QMouseEvent* event, InteractionState& interaction) override;

  void onKeyPressEvent(QKeyEvent* event, InteractionState& interaction) override;

  void onKeyReleaseEvent(QKeyEvent* event, InteractionState& interaction) override;

  void onContextMenuEvent(QContextMenuEvent* event, InteractionState& interaction) override;

 private:
  ZoneInteractionContext& m_rContext;
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
  SplineVertex::Ptr m_ptrNearestVertex;
  EditableSpline::Ptr m_ptrNearestVertexSpline;

  // These are valid if m_segmentProximity is the proximity leader.
  SplineSegment m_nearestSegment;
  EditableSpline::Ptr m_ptrNearestSegmentSpline;
  QPointF m_screenPointOnSegment;
  EditableSpline::Ptr m_ptrUnderCursorSpline;
};


#endif  // ifndef ZONE_DEFAULT_INTERACTION_H_
