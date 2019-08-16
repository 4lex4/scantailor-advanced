// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "ZoneDragInteraction.h"
#include <QMouseEvent>
#include <QPainter>
#include "ImageViewBase.h"
#include "ZoneInteractionContext.h"

ZoneDragInteraction::ZoneDragInteraction(ZoneInteractionContext& context,
                                         InteractionState& interaction,
                                         const EditableSpline::Ptr& spline)
    : m_context(context), m_spline(spline) {
  m_initialMousePos = m_context.imageView().mapFromGlobal(QCursor::pos()) + QPointF(0.5, 0.5);

  const QTransform to_screen(m_context.imageView().imageToWidget());
  m_initialSplineFirstVertexPos = to_screen.map(spline->firstVertex()->point());

  m_interaction.setInteractionStatusTip(tr("Release left mouse button to finish dragging."));
  m_interaction.setInteractionCursor(Qt::DragMoveCursor);
  interaction.capture(m_interaction);
}

void ZoneDragInteraction::onPaint(QPainter& painter, const InteractionState& interaction) {
  painter.setWorldMatrixEnabled(false);
  painter.setRenderHint(QPainter::Antialiasing);

  const QTransform to_screen(m_context.imageView().imageToWidget());

  for (const EditableZoneSet::Zone& zone : m_context.zones()) {
    const EditableSpline::Ptr& spline = zone.spline();

    if (spline != m_spline) {
      // Draw the whole spline in solid color.
      m_visualizer.drawSpline(painter, to_screen, spline);
      continue;
    }
    // Draw the solid part of the spline.
    QPolygonF points;
    for (SplineVertex::Ptr vertex(m_spline->firstVertex()); vertex != m_spline->firstVertex();
         vertex = vertex->next(SplineVertex::LOOP)) {
      points.push_back(to_screen.map(vertex->point()));
    }

    m_visualizer.prepareForSpline(painter, spline);
    painter.drawPolyline(points);

    m_visualizer.drawSpline(painter, to_screen, spline);
  }
}  // ZoneDragInteraction::onPaint

void ZoneDragInteraction::onMouseReleaseEvent(QMouseEvent* event, InteractionState& interaction) {
  if (event->button() == Qt::LeftButton) {
    m_context.zones().commit();
    makePeerPreceeder(*m_context.createDefaultInteraction());
    delete this;
  }
}

void ZoneDragInteraction::onMouseMoveEvent(QMouseEvent* event, InteractionState& interaction) {
  const QTransform to_screen(m_context.imageView().imageToWidget());
  const QTransform from_screen(m_context.imageView().widgetToImage());
  const QPointF shift = (event->pos() + QPointF(0.5, 0.5)) - m_initialMousePos;
  const QPointF splineShift = to_screen.map(m_spline->firstVertex()->point()) - m_initialSplineFirstVertexPos;

  SplineVertex::Ptr vertex(m_spline->firstVertex());
  do {
    vertex->setPoint(from_screen.map(to_screen.map(vertex->point()) + shift - splineShift));
  } while (vertex = vertex->next(SplineVertex::LOOP), vertex != m_spline->firstVertex());

  m_context.imageView().update();
}  // ZoneDragInteraction::onMouseMoveEvent
