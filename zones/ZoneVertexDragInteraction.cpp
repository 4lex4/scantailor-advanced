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

#include "ZoneVertexDragInteraction.h"
#include <QMouseEvent>
#include <QPainter>
#include "ImageViewBase.h"
#include "ZoneInteractionContext.h"

ZoneVertexDragInteraction::ZoneVertexDragInteraction(ZoneInteractionContext& context,
                                                     InteractionState& interaction,
                                                     const EditableSpline::Ptr& spline,
                                                     const SplineVertex::Ptr& vertex)
    : m_rContext(context), m_ptrSpline(spline), m_ptrVertex(vertex) {
  const QPointF screen_mouse_pos(m_rContext.imageView().mapFromGlobal(QCursor::pos()) + QPointF(0.5, 0.5));
  const QTransform to_screen(m_rContext.imageView().imageToWidget());
  m_dragOffset = to_screen.map(vertex->point()) - screen_mouse_pos;

  interaction.capture(m_interaction);
  checkProximity(interaction);
}

void ZoneVertexDragInteraction::onPaint(QPainter& painter, const InteractionState& interaction) {
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
    SplineVertex::Ptr vertex(m_ptrVertex->next(SplineVertex::LOOP));
    for (; vertex != m_ptrVertex; vertex = vertex->next(SplineVertex::LOOP)) {
      points.push_back(to_screen.map(vertex->point()));
    }

    m_visualizer.prepareForSpline(painter, spline);
    painter.drawPolyline(points);
  }

  QLinearGradient gradient;  // From remote to selected point.
  gradient.setColorAt(0.0, m_visualizer.solidColor());
  gradient.setColorAt(1.0, m_visualizer.highlightDarkColor());

  QPen gradient_pen;
  gradient_pen.setCosmetic(true);
  gradient_pen.setWidthF(1.5);

  painter.setBrush(Qt::NoBrush);

  const QPointF pt(to_screen.map(m_ptrVertex->point()));
  const QPointF prev(to_screen.map(m_ptrVertex->prev(SplineVertex::LOOP)->point()));
  const QPointF next(to_screen.map(m_ptrVertex->next(SplineVertex::LOOP)->point()));

  gradient.setStart(prev);
  gradient.setFinalStop(pt);
  gradient_pen.setBrush(gradient);
  painter.setPen(gradient_pen);
  painter.drawLine(prev, pt);

  gradient.setStart(next);
  gradient_pen.setBrush(gradient);
  painter.setPen(gradient_pen);
  painter.drawLine(next, pt);

  m_visualizer.drawVertex(painter, to_screen.map(m_ptrVertex->point()), m_visualizer.highlightBrightColor());
}  // ZoneVertexDragInteraction::onPaint

void ZoneVertexDragInteraction::onMouseReleaseEvent(QMouseEvent* event, InteractionState& interaction) {
  if (event->button() == Qt::LeftButton) {
    if ((m_ptrVertex->point() == m_ptrVertex->next(SplineVertex::LOOP)->point())
        || (m_ptrVertex->point() == m_ptrVertex->prev(SplineVertex::LOOP)->point())) {
      if (m_ptrVertex->hasAtLeastSiblings(3)) {
        m_ptrVertex->remove();
      }
    }

    m_rContext.zones().commit();
    makePeerPreceeder(*m_rContext.createDefaultInteraction());
    delete this;
  }
}

void ZoneVertexDragInteraction::onMouseMoveEvent(QMouseEvent* event, InteractionState& interaction) {
  const QTransform from_screen(m_rContext.imageView().widgetToImage());
  m_ptrVertex->setPoint(from_screen.map(event->pos() + QPointF(0.5, 0.5) + m_dragOffset));

  checkProximity(interaction);

  const Qt::KeyboardModifiers mask = event->modifiers();
  if (mask == Qt::ControlModifier) {
    const QTransform to_screen(m_rContext.imageView().imageToWidget());

    const QPointF current = to_screen.map(m_ptrVertex->point());
    QPointF prev = to_screen.map(m_ptrVertex->prev(SplineVertex::LOOP)->point());
    QPointF next = to_screen.map(m_ptrVertex->next(SplineVertex::LOOP)->point());

    if (!((current == prev) && (current == next))) {
      double prev_angle_cos
          = std::abs((prev.x() - current.x())
                     / std::sqrt(std::pow((prev.y() - current.y()), 2) + std::pow((prev.x() - current.x()), 2)));
      double next_angle_cos
          = std::abs((next.x() - current.x())
                     / std::sqrt(std::pow((next.y() - current.y()), 2) + std::pow((next.x() - current.x()), 2)));


      if ((prev_angle_cos < next_angle_cos) || (std::isnan(prev_angle_cos) && (next_angle_cos > (1.0 / std::sqrt(2))))
          || (std::isnan(next_angle_cos) && (prev_angle_cos < (1.0 / std::sqrt(2))))) {
        prev.setX(current.x());
        next.setY(current.y());
      } else {
        next.setX(current.x());
        prev.setY(current.y());
      }

      m_ptrVertex->prev(SplineVertex::LOOP)->setPoint(from_screen.map(prev));
      m_ptrVertex->next(SplineVertex::LOOP)->setPoint(from_screen.map(next));
    }
  }

  m_rContext.imageView().update();
}  // ZoneVertexDragInteraction::onMouseMoveEvent

void ZoneVertexDragInteraction::checkProximity(const InteractionState& interaction) {
  bool can_merge = false;

  if (m_ptrVertex->hasAtLeastSiblings(3)) {
    const QTransform to_screen(m_rContext.imageView().imageToWidget());
    const QPointF origin(to_screen.map(m_ptrVertex->point()));

    const QPointF prev(m_ptrVertex->prev(SplineVertex::LOOP)->point());
    const Proximity prox_prev(origin, to_screen.map(prev));

    const QPointF next(m_ptrVertex->next(SplineVertex::LOOP)->point());
    const Proximity prox_next(origin, to_screen.map(next));

    if ((prox_prev <= interaction.proximityThreshold()) && (prox_prev < prox_next)) {
      m_ptrVertex->setPoint(prev);
      can_merge = true;
    } else if (prox_next <= interaction.proximityThreshold()) {
      m_ptrVertex->setPoint(next);
      can_merge = true;
    }
  }

  if (can_merge) {
    m_interaction.setInteractionStatusTip(tr("Merge these two vertices."));
  } else {
    m_interaction.setInteractionStatusTip(tr("Move the vertex to one of its neighbors to merge them."));
  }
}
