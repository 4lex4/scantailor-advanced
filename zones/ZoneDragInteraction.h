
#ifndef SCANTAILOR_ZONEDRAGINTERACTION_H
#define SCANTAILOR_ZONEDRAGINTERACTION_H

#include "BasicSplineVisualizer.h"
#include "EditableSpline.h"
#include "InteractionHandler.h"
#include "InteractionState.h"
#include <QPointF>
#include <QCoreApplication>

class ZoneInteractionContext;

class ZoneDragInteraction : public InteractionHandler {
Q_DECLARE_TR_FUNCTIONS(ZoneDragInteraction)
public:
    ZoneDragInteraction(ZoneInteractionContext& context,
                        InteractionState& interaction,
                        EditableSpline::Ptr const& spline);

protected:
    virtual void onPaint(QPainter& painter, InteractionState const& interaction);

    virtual void onMouseReleaseEvent(QMouseEvent* event, InteractionState& interaction);

    virtual void onMouseMoveEvent(QMouseEvent* event, InteractionState& interaction);

    ZoneInteractionContext& m_rContext;
    EditableSpline::Ptr m_ptrSpline;
    QPointF m_initialMousePos;
    QPointF m_initialSplineFirstVertexPos;
    InteractionState::Captor m_interaction;
    BasicSplineVisualizer m_visualizer;
};


#endif //SCANTAILOR_ZONEDRAGINTERACTION_H
