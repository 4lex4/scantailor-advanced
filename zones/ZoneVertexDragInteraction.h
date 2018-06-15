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

#ifndef ZONE_VERTEX_DRAG_INTERACTION_H_
#define ZONE_VERTEX_DRAG_INTERACTION_H_

#include <QCoreApplication>
#include <QPointF>
#include "BasicSplineVisualizer.h"
#include "EditableSpline.h"
#include "InteractionHandler.h"
#include "InteractionState.h"

class ZoneInteractionContext;

class ZoneVertexDragInteraction : public InteractionHandler {
  Q_DECLARE_TR_FUNCTIONS(ZoneVertexDragInteraction)
 public:
  ZoneVertexDragInteraction(ZoneInteractionContext& context,
                            InteractionState& interaction,
                            const EditableSpline::Ptr& spline,
                            const SplineVertex::Ptr& vertex);

 protected:
  void onPaint(QPainter& painter, const InteractionState& interaction) override;

  void onMouseReleaseEvent(QMouseEvent* event, InteractionState& interaction) override;

  void onMouseMoveEvent(QMouseEvent* event, InteractionState& interaction) override;

 private:
  void checkProximity(const InteractionState& interaction);

  ZoneInteractionContext& m_rContext;
  EditableSpline::Ptr m_ptrSpline;
  SplineVertex::Ptr m_ptrVertex;
  InteractionState::Captor m_interaction;
  BasicSplineVisualizer m_visualizer;
  QPointF m_dragOffset;
};


#endif  // ifndef ZONE_VERTEX_DRAG_INTERACTION_H_
