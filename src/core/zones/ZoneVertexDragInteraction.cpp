// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "ZoneVertexDragInteraction.h"

#include <QMouseEvent>
#include <QPainter>

#include "ImageViewBase.h"
#include "ZoneInteractionContext.h"

ZoneVertexDragInteraction::ZoneVertexDragInteraction(ZoneInteractionContext& context,
                                                     InteractionState& interaction,
                                                     const EditableSpline::Ptr& spline,
                                                     const SplineVertex::Ptr& vertex)
    : m_context(context), m_spline(spline), m_vertex(vertex) {
  const QPointF screenMousePos(m_context.imageView().mapFromGlobal(QCursor::pos()) + QPointF(0.5, 0.5));
  const QTransform toScreen(m_context.imageView().imageToWidget());
  m_dragOffset = toScreen.map(vertex->point()) - screenMousePos;

  interaction.capture(m_interaction);
  checkProximity(interaction);
}

void ZoneVertexDragInteraction::onPaint(QPainter& painter, const InteractionState& interaction) {
  painter.setWorldMatrixEnabled(false);
  painter.setRenderHint(QPainter::Antialiasing);

  const QTransform toScreen(m_context.imageView().imageToWidget());

  for (const EditableZoneSet::Zone& zone : m_context.zones()) {
    const EditableSpline::Ptr& spline = zone.spline();

    if (spline != m_spline) {
      // Draw the whole spline in solid color.
      m_visualizer.drawSpline(painter, toScreen, spline);
      continue;
    }
    // Draw the solid part of the spline.
    QPolygonF points;
    SplineVertex::Ptr vertex(m_vertex->next(SplineVertex::LOOP));
    for (; vertex != m_vertex; vertex = vertex->next(SplineVertex::LOOP)) {
      points.push_back(toScreen.map(vertex->point()));
    }

    m_visualizer.prepareForSpline(painter, spline);
    painter.drawPolyline(points);
  }

  QLinearGradient gradient;  // From remote to selected point.
  gradient.setColorAt(0.0, m_visualizer.solidColor());
  gradient.setColorAt(1.0, m_visualizer.highlightDarkColor());

  QPen gradientPen;
  gradientPen.setCosmetic(true);
  gradientPen.setWidthF(1.5);

  painter.setBrush(Qt::NoBrush);

  const QPointF pt(toScreen.map(m_vertex->point()));
  const QPointF prev(toScreen.map(m_vertex->prev(SplineVertex::LOOP)->point()));
  const QPointF next(toScreen.map(m_vertex->next(SplineVertex::LOOP)->point()));

  gradient.setStart(prev);
  gradient.setFinalStop(pt);
  gradientPen.setBrush(gradient);
  painter.setPen(gradientPen);
  painter.drawLine(prev, pt);

  gradient.setStart(next);
  gradientPen.setBrush(gradient);
  painter.setPen(gradientPen);
  painter.drawLine(next, pt);

  m_visualizer.drawVertex(painter, toScreen.map(m_vertex->point()), m_visualizer.highlightBrightColor());
}  // ZoneVertexDragInteraction::onPaint

void ZoneVertexDragInteraction::onMouseReleaseEvent(QMouseEvent* event, InteractionState& interaction) {
  if (event->button() == Qt::LeftButton) {
    if ((m_vertex->point() == m_vertex->next(SplineVertex::LOOP)->point())
        || (m_vertex->point() == m_vertex->prev(SplineVertex::LOOP)->point())) {
      if (m_vertex->hasAtLeastSiblings(3)) {
        m_vertex->remove();
      }
    }

    m_context.zones().commit();
    makePeerPreceeder(*m_context.createDefaultInteraction());
    delete this;
  }
}

void ZoneVertexDragInteraction::onMouseMoveEvent(QMouseEvent* event, InteractionState& interaction) {
  const QTransform fromScreen(m_context.imageView().widgetToImage());
  m_vertex->setPoint(fromScreen.map(event->pos() + QPointF(0.5, 0.5) + m_dragOffset));

  checkProximity(interaction);

  const Qt::KeyboardModifiers mask = event->modifiers();
  if (mask == Qt::ControlModifier) {
    const QTransform toScreen(m_context.imageView().imageToWidget());

    const QPointF current = toScreen.map(m_vertex->point());
    QPointF prev = toScreen.map(m_vertex->prev(SplineVertex::LOOP)->point());
    QPointF next = toScreen.map(m_vertex->next(SplineVertex::LOOP)->point());

    if (!((current == prev) && (current == next))) {
      double prevAngleCos
          = std::abs((prev.x() - current.x())
                     / std::sqrt(std::pow((prev.y() - current.y()), 2) + std::pow((prev.x() - current.x()), 2)));
      double nextAngleCos
          = std::abs((next.x() - current.x())
                     / std::sqrt(std::pow((next.y() - current.y()), 2) + std::pow((next.x() - current.x()), 2)));


      if ((prevAngleCos < nextAngleCos) || (std::isnan(prevAngleCos) && (nextAngleCos > (1.0 / std::sqrt(2))))
          || (std::isnan(nextAngleCos) && (prevAngleCos < (1.0 / std::sqrt(2))))) {
        prev.setX(current.x());
        next.setY(current.y());
      } else {
        next.setX(current.x());
        prev.setY(current.y());
      }

      m_vertex->prev(SplineVertex::LOOP)->setPoint(fromScreen.map(prev));
      m_vertex->next(SplineVertex::LOOP)->setPoint(fromScreen.map(next));
    }
  }

  m_context.imageView().update();
}  // ZoneVertexDragInteraction::onMouseMoveEvent

void ZoneVertexDragInteraction::checkProximity(const InteractionState& interaction) {
  bool canMerge = false;

  if (m_vertex->hasAtLeastSiblings(3)) {
    const QTransform toScreen(m_context.imageView().imageToWidget());
    const QPointF origin(toScreen.map(m_vertex->point()));

    const QPointF prev(m_vertex->prev(SplineVertex::LOOP)->point());
    const Proximity proxPrev(origin, toScreen.map(prev));

    const QPointF next(m_vertex->next(SplineVertex::LOOP)->point());
    const Proximity proxNext(origin, toScreen.map(next));

    if ((proxPrev <= interaction.proximityThreshold()) && (proxPrev < proxNext)) {
      m_vertex->setPoint(prev);
      canMerge = true;
    } else if (proxNext <= interaction.proximityThreshold()) {
      m_vertex->setPoint(next);
      canMerge = true;
    }
  }

  if (canMerge) {
    m_interaction.setInteractionStatusTip(tr("Merge these two vertices."));
  } else {
    m_interaction.setInteractionStatusTip(tr("Move the vertex to one of its neighbors to merge them."));
  }
}
