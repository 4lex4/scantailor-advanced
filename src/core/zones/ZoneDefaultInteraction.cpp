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
  m_zoneAreaDragProximity.setProximityStatusTip("Hold left mouse button to drag the zone.");
  m_zoneAreaDragProximity.setProximityCursor(Qt::DragMoveCursor);
  m_zoneAreaDragCopyProximity.setProximityStatusTip("Hold left mouse button to copy and drag the zone.");
  m_zoneAreaDragCopyProximity.setProximityCursor(Qt::DragCopyCursor);
  m_context.imageView().interactionState().setDefaultStatusTip(
      tr("Click to start creating a new zone. Use Ctrl+Alt+Click to copy the latest created zone."));
}

void ZoneDefaultInteraction::onPaint(QPainter& painter, const InteractionState& interaction) {
  painter.setWorldMatrixEnabled(false);
  painter.setRenderHint(QPainter::Antialiasing);

  const QTransform to_screen(m_context.imageView().imageToWidget());

  for (const EditableZoneSet::Zone& zone : m_context.zones()) {
    const EditableSpline::Ptr& spline = zone.spline();
    m_visualizer.prepareForSpline(painter, spline);
    QPolygonF points;

    if (!interaction.captured() && interaction.proximityLeader(m_vertexProximity)
        && (spline == m_nearestVertexSpline)) {
      SplineVertex::Ptr vertex(m_nearestVertex->next(SplineVertex::LOOP));
      for (; vertex != m_nearestVertex; vertex = vertex->next(SplineVertex::LOOP)) {
        points.push_back(to_screen.map(vertex->point()));
      }
      painter.drawPolyline(points);
    } else if (!interaction.captured() && interaction.proximityLeader(m_segmentProximity)
               && (spline == m_nearestSegmentSpline)) {
      SplineVertex::Ptr vertex(m_nearestSegment.prev);
      do {
        vertex = vertex->next(SplineVertex::LOOP);
        points.push_back(to_screen.map(vertex->point()));
      } while (vertex != m_nearestSegment.prev);
      painter.drawPolyline(points);
    } else {
      m_visualizer.drawSpline(painter, to_screen, spline);
    }
  }

  if (interaction.proximityLeader(m_vertexProximity)) {
    // Draw the two adjacent edges in gradient red-to-orange.
    QLinearGradient gradient;  // From inactive to active point.
    gradient.setColorAt(0.0, m_visualizer.solidColor());
    gradient.setColorAt(1.0, m_visualizer.highlightDarkColor());

    QPen pen(painter.pen());

    const QPointF prev(to_screen.map(m_nearestVertex->prev(SplineVertex::LOOP)->point()));
    const QPointF pt(to_screen.map(m_nearestVertex->point()));
    const QPointF next(to_screen.map(m_nearestVertex->next(SplineVertex::LOOP)->point()));

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
    const QPointF screen_vertex(to_screen.map(m_nearestVertex->point()));
    m_visualizer.drawVertex(painter, screen_vertex, m_visualizer.highlightBrightColor());
  } else if (interaction.proximityLeader(m_segmentProximity)) {
    const QLineF line(to_screen.map(m_nearestSegment.toLine()));
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

void ZoneDefaultInteraction::onProximityUpdate(const QPointF& mouse_pos, InteractionState& interaction) {
  m_screenMousePos = mouse_pos;

  const QTransform to_screen(m_context.imageView().imageToWidget());
  const QTransform from_screen(m_context.imageView().widgetToImage());
  const QPointF image_mouse_pos(from_screen.map(mouse_pos));

  m_nearestVertex.reset();
  m_nearestVertexSpline.reset();
  m_nearestSegment = SplineSegment();
  m_nearestSegmentSpline.reset();
  m_underCursorSpline.reset();

  Proximity best_vertex_proximity;
  Proximity best_segment_proximity;

  bool has_zone_under_mouse = false;

  for (const EditableZoneSet::Zone& zone : m_context.zones()) {
    const EditableSpline::Ptr& spline = zone.spline();

    {
      QPainterPath path;
      path.setFillRule(Qt::WindingFill);
      path.addPolygon(spline->toPolygon());
      if (path.contains(image_mouse_pos)) {
        m_underCursorSpline = spline;

        has_zone_under_mouse = true;
      }
    }

    // Process vertices.
    for (SplineVertex::Ptr vert(spline->firstVertex()); vert; vert = vert->next(SplineVertex::NO_LOOP)) {
      const Proximity proximity(mouse_pos, to_screen.map(vert->point()));
      if (proximity < best_vertex_proximity) {
        m_nearestVertex = vert;
        m_nearestVertexSpline = spline;
        best_vertex_proximity = proximity;
      }
    }
    // Process segments.
    for (EditableSpline::SegmentIterator it(*spline); it.hasNext();) {
      const SplineSegment segment(it.next());
      const QLineF line(to_screen.map(segment.toLine()));
      QPointF point_on_segment;
      const Proximity proximity(Proximity::pointAndLineSegment(mouse_pos, line, &point_on_segment));
      if (proximity < best_segment_proximity) {
        m_nearestSegment = segment;
        m_nearestSegmentSpline = spline;
        best_segment_proximity = proximity;
        m_screenPointOnSegment = point_on_segment;
      }
    }
  }

  interaction.updateProximity(m_vertexProximity, best_vertex_proximity, 2);
  interaction.updateProximity(m_segmentProximity, best_segment_proximity, 1);

  if (has_zone_under_mouse) {
    const Proximity zone_area_proximity(std::min(best_vertex_proximity, best_segment_proximity));
    interaction.updateProximity(m_zoneAreaProximity, zone_area_proximity, -1, zone_area_proximity);
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
    const QTransform from_screen(m_context.imageView().widgetToImage());
    SplineVertex::Ptr vertex(m_nearestSegment.splitAt(from_screen.map(m_screenPointOnSegment)));
    makePeerPreceeder(*m_context.createVertexDragInteraction(interaction, m_nearestSegmentSpline, vertex));
    delete this;
    event->accept();
  } else if (interaction.proximityLeader(m_zoneAreaDragProximity)) {
    makePeerPreceeder(*m_context.createZoneDragInteraction(interaction, m_underCursorSpline));
    delete this;
    event->accept();
  } else if (interaction.proximityLeader(m_zoneAreaDragCopyProximity)) {
    auto new_spline = make_intrusive<EditableSpline>(SerializableSpline(*m_underCursorSpline));
    m_context.zones().addZone(new_spline, *m_context.zones().propertiesFor(m_underCursorSpline));
    makePeerPreceeder(*m_context.createZoneDragInteraction(interaction, new_spline));
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
    const QTransform from_screen(m_context.imageView().widgetToImage());

    EditableZoneSet::const_iterator latest_zone = --m_context.zones().end();
    if (latest_zone != m_context.zones().end()) {
      SerializableSpline serializable_spline(*(*latest_zone).spline());

      const QPointF old_center = serializable_spline.toPolygon().boundingRect().center();
      const QPointF new_center = from_screen.map(event->pos() + QPointF(0.5, 0.5));
      const QPointF shift = new_center - old_center;

      serializable_spline = serializable_spline.transformed(QTransform().translate(shift.x(), shift.y()));

      auto new_spline = make_intrusive<EditableSpline>(serializable_spline);
      m_context.zones().addZone(new_spline, *(*latest_zone).properties());
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
  const QTransform to_screen(m_context.imageView().imageToWidget());

  m_screenMousePos = to_screen.map(event->pos() + QPointF(0.5, 0.5));
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

  InteractionHandler* cm_interaction = m_context.createContextMenuInteraction(interaction);
  if (!cm_interaction) {
    return;
  }

  makePeerPreceeder(*cm_interaction);
  delete this;
}
