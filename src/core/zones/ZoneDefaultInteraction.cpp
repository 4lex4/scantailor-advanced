// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "ZoneDefaultInteraction.h"

#include <QMouseEvent>
#include <QPainter>

#include "ImageViewBase.h"
#include "SerializableSpline.h"
#include "ZoneInteractionContext.h"

ZoneDefaultInteraction::ZoneDefaultInteraction(ZoneInteractionContext& context)
    : m_context(context), m_dragHandler(context.imageView()), m_dragWatcher(m_dragHandler) {
  makeLastFollower(m_dragHandler);
  m_dragHandler.makeFirstFollower(m_dragWatcher);

  m_vertexProximity.setProximityStatusTip(tr("Drag the vertex. Hold Ctrl to make the vertex angle right."));
  m_segmentProximity.setProximityStatusTip(tr("Click to create a new vertex here."));
  m_zoneAreaProximity.setProximityStatusTip(
      tr("Right click to edit zone properties. Hold Shift to drag the zone or "
         "Shift+Ctrl to copy. Press Del to delete this zone."));
  m_zoneAreaDragProximity.setProximityStatusTip(tr("Hold left mouse button to drag the zone."));
  m_zoneAreaDragProximity.setProximityCursor(Qt::DragMoveCursor);
  m_zoneAreaDragCopyProximity.setProximityStatusTip(tr("Hold left mouse button to copy and drag the zone."));
  m_zoneAreaDragCopyProximity.setProximityCursor(Qt::DragCopyCursor);
  m_context.imageView().interactionState().setDefaultStatusTip(
      tr("Click to start creating a new zone. Use Ctrl+Alt+Click to copy the latest created zone."));
}

void ZoneDefaultInteraction::onPaint(QPainter& painter, const InteractionState& interaction) {
  painter.setWorldMatrixEnabled(false);
  painter.setRenderHint(QPainter::Antialiasing);

  const QTransform toScreen(m_context.imageView().imageToWidget());

  for (const EditableZoneSet::Zone& zone : m_context.zones()) {
    const EditableSpline::Ptr& spline = zone.spline();
    m_visualizer.prepareForSpline(painter, spline);
    QPolygonF points;

    if (!interaction.captured() && interaction.proximityLeader(m_vertexProximity)
        && (spline == m_nearestVertexSpline)) {
      SplineVertex::Ptr vertex(m_nearestVertex->next(SplineVertex::LOOP));
      for (; vertex != m_nearestVertex; vertex = vertex->next(SplineVertex::LOOP)) {
        points.push_back(toScreen.map(vertex->point()));
      }
      painter.drawPolyline(points);
    } else if (!interaction.captured() && interaction.proximityLeader(m_segmentProximity)
               && (spline == m_nearestSegmentSpline)) {
      SplineVertex::Ptr vertex(m_nearestSegment.prev);
      do {
        vertex = vertex->next(SplineVertex::LOOP);
        points.push_back(toScreen.map(vertex->point()));
      } while (vertex != m_nearestSegment.prev);
      painter.drawPolyline(points);
    } else {
      m_visualizer.drawSpline(painter, toScreen, spline);
    }
  }

  if (interaction.proximityLeader(m_vertexProximity)) {
    // Draw the two adjacent edges in gradient red-to-orange.
    QLinearGradient gradient;  // From inactive to active point.
    gradient.setColorAt(0.0, m_visualizer.solidColor());
    gradient.setColorAt(1.0, m_visualizer.highlightDarkColor());

    QPen pen(painter.pen());

    const QPointF prev(toScreen.map(m_nearestVertex->prev(SplineVertex::LOOP)->point()));
    const QPointF pt(toScreen.map(m_nearestVertex->point()));
    const QPointF next(toScreen.map(m_nearestVertex->next(SplineVertex::LOOP)->point()));

    gradient.setStart(prev);
    gradient.setFinalStop(pt);
    pen.setBrush(gradient);
    painter.setPen(pen);
    painter.drawLine(prev, pt);

    gradient.setStart(next);
    pen.setBrush(gradient);
    painter.setPen(pen);
    painter.drawLine(next, pt);

    // Visualize the highlighted vertex.
    const QPointF screenVertex(toScreen.map(m_nearestVertex->point()));
    m_visualizer.drawVertex(painter, screenVertex, m_visualizer.highlightBrightColor());
  } else if (interaction.proximityLeader(m_segmentProximity)) {
    const QLineF line(toScreen.map(m_nearestSegment.toLine()));
    // Draw the highglighed edge in orange.
    QPen pen(painter.pen());
    pen.setColor(m_visualizer.highlightDarkColor());
    painter.setPen(pen);
    painter.drawLine(line);

    m_visualizer.drawVertex(painter, m_screenPointOnSegment, m_visualizer.highlightBrightColor());
  } else if (!interaction.captured()) {
    m_visualizer.drawVertex(painter, m_screenMousePos, m_visualizer.solidColor());
  }
}  // ZoneDefaultInteraction::onPaint

void ZoneDefaultInteraction::onProximityUpdate(const QPointF& mousePos, InteractionState& interaction) {
  m_screenMousePos = mousePos;

  const QTransform toScreen(m_context.imageView().imageToWidget());
  const QTransform fromScreen(m_context.imageView().widgetToImage());
  const QPointF imageMousePos(fromScreen.map(mousePos));

  m_nearestVertex.reset();
  m_nearestVertexSpline.reset();
  m_nearestSegment = SplineSegment();
  m_nearestSegmentSpline.reset();
  m_underCursorSpline.reset();

  Proximity bestVertexProximity;
  Proximity bestSegmentProximity;

  bool hasZoneUnderMouse = false;

  for (const EditableZoneSet::Zone& zone : m_context.zones()) {
    const EditableSpline::Ptr& spline = zone.spline();

    {
      QPainterPath path;
      path.setFillRule(Qt::WindingFill);
      path.addPolygon(spline->toPolygon());
      if (path.contains(imageMousePos)) {
        m_underCursorSpline = spline;

        hasZoneUnderMouse = true;
      }
    }

    // Process vertices.
    for (SplineVertex::Ptr vert(spline->firstVertex()); vert; vert = vert->next(SplineVertex::NO_LOOP)) {
      const Proximity proximity(mousePos, toScreen.map(vert->point()));
      if (proximity < bestVertexProximity) {
        m_nearestVertex = vert;
        m_nearestVertexSpline = spline;
        bestVertexProximity = proximity;
      }
    }
    // Process segments.
    for (EditableSpline::SegmentIterator it(*spline); it.hasNext();) {
      const SplineSegment segment(it.next());
      const QLineF line(toScreen.map(segment.toLine()));
      QPointF pointOnSegment;
      const Proximity proximity(Proximity::pointAndLineSegment(mousePos, line, &pointOnSegment));
      if (proximity < bestSegmentProximity) {
        m_nearestSegment = segment;
        m_nearestSegmentSpline = spline;
        bestSegmentProximity = proximity;
        m_screenPointOnSegment = pointOnSegment;
      }
    }
  }

  interaction.updateProximity(m_vertexProximity, bestVertexProximity, 2);
  interaction.updateProximity(m_segmentProximity, bestSegmentProximity, 1);

  if (hasZoneUnderMouse) {
    const Proximity zoneAreaProximity(std::min(bestVertexProximity, bestSegmentProximity));
    interaction.updateProximity(m_zoneAreaProximity, zoneAreaProximity, -1, zoneAreaProximity);
    if (m_activeKeyboardModifiers == Qt::ShiftModifier) {
      interaction.updateProximity(m_zoneAreaDragProximity, Proximity::fromSqDist(0), 0);
    } else if (m_activeKeyboardModifiers == (Qt::ShiftModifier | Qt::ControlModifier)) {
      interaction.updateProximity(m_zoneAreaDragCopyProximity, Proximity::fromSqDist(0), 0);
    }
  }
}  // ZoneDefaultInteraction::onProximityUpdate

void ZoneDefaultInteraction::onMousePressEvent(QMouseEvent* event, InteractionState& interaction) {
  if (interaction.captured()) {
    return;
  }
  if (event->button() != Qt::LeftButton) {
    return;
  }

  if (interaction.proximityLeader(m_vertexProximity)) {
    makePeerPreceeder(*m_context.createVertexDragInteraction(interaction, m_nearestVertexSpline, m_nearestVertex));
    delete this;
    event->accept();
  } else if (interaction.proximityLeader(m_segmentProximity)) {
    const QTransform fromScreen(m_context.imageView().widgetToImage());
    SplineVertex::Ptr vertex(m_nearestSegment.splitAt(fromScreen.map(m_screenPointOnSegment)));
    makePeerPreceeder(*m_context.createVertexDragInteraction(interaction, m_nearestSegmentSpline, vertex));
    delete this;
    event->accept();
  } else if (interaction.proximityLeader(m_zoneAreaDragProximity)) {
    makePeerPreceeder(*m_context.createZoneDragInteraction(interaction, m_underCursorSpline));
    delete this;
    event->accept();
  } else if (interaction.proximityLeader(m_zoneAreaDragCopyProximity)) {
    auto newSpline = make_intrusive<EditableSpline>(SerializableSpline(*m_underCursorSpline));
    m_context.zones().addZone(newSpline, *m_context.zones().propertiesFor(m_underCursorSpline));
    makePeerPreceeder(*m_context.createZoneDragInteraction(interaction, newSpline));
    delete this;
    event->accept();
  }
}

void ZoneDefaultInteraction::onMouseReleaseEvent(QMouseEvent* event, InteractionState& interaction) {
  if (event->button() != Qt::LeftButton) {
    return;
  }

  if (!interaction.captured()) {
    return;
  }
  if (!m_dragHandler.isActive() || m_dragWatcher.haveSignificantDrag()) {
    return;
  }

  if (m_activeKeyboardModifiers == (Qt::ControlModifier | Qt::AltModifier)) {
    const QTransform fromScreen(m_context.imageView().widgetToImage());

    EditableZoneSet::const_iterator latest_zone = --m_context.zones().end();
    if (latest_zone != m_context.zones().end()) {
      SerializableSpline serializableSpline(*(*latest_zone).spline());

      const QPointF oldCenter = serializableSpline.toPolygon().boundingRect().center();
      const QPointF newCenter = fromScreen.map(event->pos() + QPointF(0.5, 0.5));
      const QPointF shift = newCenter - oldCenter;

      serializableSpline = serializableSpline.transformed(QTransform().translate(shift.x(), shift.y()));

      auto newSpline = make_intrusive<EditableSpline>(serializableSpline);
      m_context.zones().addZone(newSpline, *(*latest_zone).properties());
      m_context.zones().commit();
    }

    m_context.imageView().update();
    return;
  }

  makePeerPreceeder(*m_context.createZoneCreationInteraction(interaction));
  delete this;
  event->accept();
}

void ZoneDefaultInteraction::onMouseMoveEvent(QMouseEvent* event, InteractionState& interaction) {
  const QTransform toScreen(m_context.imageView().imageToWidget());

  m_screenMousePos = toScreen.map(event->pos() + QPointF(0.5, 0.5));
  m_context.imageView().update();
}

void ZoneDefaultInteraction::onKeyPressEvent(QKeyEvent* event, InteractionState& interaction) {
  m_activeKeyboardModifiers = event->modifiers();
}

void ZoneDefaultInteraction::onKeyReleaseEvent(QKeyEvent* event, InteractionState& interaction) {
  m_activeKeyboardModifiers = event->modifiers();

  if (event->key() == Qt::Key_Delete) {
    if (m_underCursorSpline != nullptr) {
      m_context.zones().removeZone(m_underCursorSpline);
      m_context.zones().commit();
    }

    m_context.imageView().update();
  }
}

void ZoneDefaultInteraction::onContextMenuEvent(QContextMenuEvent* event, InteractionState& interaction) {
  event->accept();

  InteractionHandler* cmInteraction = m_context.createContextMenuInteraction(interaction);
  if (!cmInteraction) {
    return;
  }

  makePeerPreceeder(*cmInteraction);
  delete this;
}
