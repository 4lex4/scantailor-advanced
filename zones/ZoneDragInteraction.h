
#ifndef SCANTAILOR_ZONEDRAGINTERACTION_H
#define SCANTAILOR_ZONEDRAGINTERACTION_H

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

  ZoneInteractionContext& m_rContext;
  EditableSpline::Ptr m_ptrSpline;
  QPointF m_initialMousePos;
  QPointF m_initialSplineFirstVertexPos;
  InteractionState::Captor m_interaction;
  BasicSplineVisualizer m_visualizer;
};


#endif  // SCANTAILOR_ZONEDRAGINTERACTION_H
