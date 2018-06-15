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

#include "ZoneDragInteraction.h"
#include <QMouseEvent>
#include <QPainter>
#include "ImageViewBase.h"
#include "ZoneInteractionContext.h"

ZoneDragInteraction::ZoneDragInteraction(ZoneInteractionContext& context,
                                         InteractionState& interaction,
                                         const EditableSpline::Ptr& spline)
    : m_rContext(context), m_ptrSpline(spline) {
  m_initialMousePos = m_rContext.imageView().mapFromGlobal(QCursor::pos()) + QPointF(0.5, 0.5);

  const QTransform to_screen(m_rContext.imageView().imageToWidget());
  m_initialSplineFirstVertexPos = to_screen.map(spline->firstVertex()->point());

  m_interaction.setInteractionStatusTip(tr("Release left mouse button to finish dragging."));
  m_interaction.setInteractionCursor(Qt::DragMoveCursor);
  interaction.capture(m_interaction);
}

void ZoneDragInteraction::onPaint(QPainter& painter, const InteractionState& interaction) {
  painter.setWorldMatrixEnabled(false);
  painter.setRenderHint(QPainter::Antialiasing);

  const QTransform to_screen(m_rContext.imageView().imageToWidget());

  for (const EditableZoneSet::Zone& zone : m_rContext.zones()) {
    const EditableSpline::Ptr& spline = zone.spline();

    if (spline != m_ptrSpline) {
      // Draw the whole spline in solid color.
      m_visualizer.drawSpline(painter, to_screen, spline);
      continue;
    }
    // Draw the solid part of the spline.
    QPolygonF points;
    for (SplineVertex::Ptr vertex(m_ptrSpline->firstVertex()); vertex != m_ptrSpline->firstVertex();
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
    m_rContext.zones().commit();
    makePeerPreceeder(*m_rContext.createDefaultInteraction());
    delete this;
  }
}

void ZoneDragInteraction::onMouseMoveEvent(QMouseEvent* event, InteractionState& interaction) {
  const QTransform to_screen(m_rContext.imageView().imageToWidget());
  const QTransform from_screen(m_rContext.imageView().widgetToImage());
  const QPointF shift = (event->pos() + QPointF(0.5, 0.5)) - m_initialMousePos;
  const QPointF splineShift = to_screen.map(m_ptrSpline->firstVertex()->point()) - m_initialSplineFirstVertexPos;

  SplineVertex::Ptr vertex(m_ptrSpline->firstVertex());
  do {
    vertex->setPoint(from_screen.map(to_screen.map(vertex->point()) + shift - splineShift));
  } while (vertex = vertex->next(SplineVertex::LOOP), vertex != m_ptrSpline->firstVertex());

  m_rContext.imageView().update();
}  // ZoneDragInteraction::onMouseMoveEvent
