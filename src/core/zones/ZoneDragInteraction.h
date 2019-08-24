// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_ZONES_ZONEDRAGINTERACTION_H_
#define SCANTAILOR_ZONES_ZONEDRAGINTERACTION_H_

#include <QCoreApplication>
#include <QPointF>
#include "BasicSplineVisualizer.h"
#include "EditableSpline.h"
#include "InteractionHandler.h"
#include "InteractionState.h"

class ZoneInteractionContext;

class ZoneDragInteraction : public InteractionHandler {
  Q_DECLARE_TR_FUNCTIONS(ZoneDragInteraction)
 public:
  ZoneDragInteraction(ZoneInteractionContext& context,
                      InteractionState& interaction,
                      const EditableSpline::Ptr& spline);

 protected:
  void onPaint(QPainter& painter, const InteractionState& interaction) override;

  void onMouseReleaseEvent(QMouseEvent* event, InteractionState& interaction) override;

  void onMouseMoveEvent(QMouseEvent* event, InteractionState& interaction) override;

  ZoneInteractionContext& m_context;
  EditableSpline::Ptr m_spline;
  QPointF m_initialMousePos;
  QPointF m_initialSplineFirstVertexPos;
  InteractionState::Captor m_interaction;
  BasicSplineVisualizer m_visualizer;
};


#endif  // SCANTAILOR_ZONES_ZONEDRAGINTERACTION_H_
