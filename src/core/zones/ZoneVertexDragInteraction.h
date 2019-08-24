// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_ZONES_ZONEVERTEXDRAGINTERACTION_H_
#define SCANTAILOR_ZONES_ZONEVERTEXDRAGINTERACTION_H_

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

  ZoneInteractionContext& m_context;
  EditableSpline::Ptr m_spline;
  SplineVertex::Ptr m_vertex;
  InteractionState::Captor m_interaction;
  BasicSplineVisualizer m_visualizer;
  QPointF m_dragOffset;
};


#endif  // ifndef SCANTAILOR_ZONES_ZONEVERTEXDRAGINTERACTION_H_
