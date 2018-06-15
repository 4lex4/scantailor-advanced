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

#include "ZoneCreationInteraction.h"
#include <QDebug>
#include <QKeyEvent>
#include <QPainter>
#include <boost/bind.hpp>
#include <boost/lambda/lambda.hpp>
#include "ImageViewBase.h"
#include "ZoneInteractionContext.h"

ZoneCreationInteraction::ZoneCreationInteraction(ZoneInteractionContext& context, InteractionState& interaction)
    : m_rContext(context),
      m_dragHandler(context.imageView(), boost::bind(&ZoneCreationInteraction::isDragHandlerPermitted, this, _1)),
      m_dragWatcher(m_dragHandler),
      m_zoomHandler(context.imageView(), boost::lambda::constant(true)),
      m_ptrSpline(new EditableSpline),
      m_lassoModeModifiers(Qt::ShiftModifier | Qt::AltModifier) {
  const QPointF screen_mouse_pos(m_rContext.imageView().mapFromGlobal(QCursor::pos()) + QPointF(0.5, 0.5));
  const QTransform from_screen(m_rContext.imageView().widgetToImage());
  m_nextVertexImagePos = from_screen.map(screen_mouse_pos);

  m_nextVertexImagePos_mid1 = m_nextVertexImagePos;
  m_nextVertexImagePos_mid2 = m_nextVertexImagePos;
  m_rectangularZoneType = false;

  m_lassoMode = false;

  makeLastFollower(m_dragHandler);
  m_dragHandler.makeFirstFollower(m_dragWatcher);

  makeLastFollower(m_zoomHandler);

  interaction.capture(m_interaction);
  m_ptrSpline->appendVertex(m_nextVertexImagePos);

  updateStatusTip();
}

void ZoneCreationInteraction::onPaint(QPainter& painter, const InteractionState& interaction) {
  painter.setWorldMatrixEnabled(false);
  painter.setRenderHint(QPainter::Antialiasing);

  const QTransform to_screen(m_rContext.imageView().imageToWidget());
  const QTransform from_screen(m_rContext.imageView().widgetToImage());

  m_visualizer.drawSplines(painter, to_screen, m_rContext.zones());

  QPen solid_line_pen(m_visualizer.solidColor());
  solid_line_pen.setCosmetic(true);
  solid_line_pen.setWidthF(1.5);

  QLinearGradient gradient;  // From inactive to active point.
  gradient.setColorAt(0.0, m_visualizer.solidColor());
  gradient.setColorAt(1.0, m_visualizer.highlightDarkColor());

  QPen gradient_pen;
  gradient_pen.setCosmetic(true);
  gradient_pen.setWidthF(1.5);

  painter.setPen(solid_line_pen);
  painter.setBrush(Qt::NoBrush);

  QColor start_color = m_visualizer.solidColor();
  QColor stop_color = m_visualizer.highlightDarkColor();
  QColor mid_color;

  int red = (start_color.red() + stop_color.red()) / 2;
  int green = (start_color.green() + stop_color.green()) / 2;
  int blue = (start_color.blue() + stop_color.blue()) / 2;

  mid_color.setRgb(red, green, blue);

  QLinearGradient gradient_mid1;
  gradient_mid1.setColorAt(0.0, start_color);
  gradient_mid1.setColorAt(1.0, mid_color);

  QLinearGradient gradient_mid2;
  gradient_mid2.setColorAt(0.0, mid_color);
  gradient_mid2.setColorAt(1.0, stop_color);

  if (m_rectangularZoneType) {
    if (m_nextVertexImagePos != m_ptrSpline->firstVertex()->point()) {
      m_visualizer.drawVertex(painter, to_screen.map(m_nextVertexImagePos_mid1), m_visualizer.highlightBrightColor());
      m_visualizer.drawVertex(painter, to_screen.map(m_nextVertexImagePos_mid2), m_visualizer.highlightBrightColor());

      const QLineF line1_mid1(to_screen.map(QLineF(m_ptrSpline->firstVertex()->point(), m_nextVertexImagePos_mid1)));
      gradient_mid1.setStart(line1_mid1.p1());
      gradient_mid1.setFinalStop(line1_mid1.p2());
      gradient_pen.setBrush(gradient_mid1);
      painter.setPen(gradient_pen);
      painter.drawLine(line1_mid1);

      const QLineF line2_mid1(to_screen.map(QLineF(m_nextVertexImagePos_mid1, m_nextVertexImagePos)));
      gradient_mid2.setStart(line2_mid1.p1());
      gradient_mid2.setFinalStop(line2_mid1.p2());
      gradient_pen.setBrush(gradient_mid2);
      painter.setPen(gradient_pen);
      painter.drawLine(line2_mid1);

      const QLineF line1_mid2(to_screen.map(QLineF(m_ptrSpline->firstVertex()->point(), m_nextVertexImagePos_mid2)));
      gradient_mid1.setStart(line1_mid2.p1());
      gradient_mid1.setFinalStop(line1_mid2.p2());
      gradient_pen.setBrush(gradient_mid1);
      painter.setPen(gradient_pen);
      painter.drawLine(line1_mid2);

      const QLineF line2_mid2(to_screen.map(QLineF(m_nextVertexImagePos_mid2, m_nextVertexImagePos)));
      gradient_mid2.setStart(line2_mid2.p1());
      gradient_mid2.setFinalStop(line2_mid2.p2());
      gradient_pen.setBrush(gradient_mid2);
      painter.setPen(gradient_pen);
      painter.drawLine(line2_mid2);
    }
  } else {
    for (EditableSpline::SegmentIterator it(*m_ptrSpline); it.hasNext();) {
      const SplineSegment segment(it.next());
      const QLineF line(to_screen.map(segment.toLine()));

      if ((segment.prev == m_ptrSpline->firstVertex()) && (segment.prev->point() == m_nextVertexImagePos)) {
        gradient.setStart(line.p2());
        gradient.setFinalStop(line.p1());
        gradient_pen.setBrush(gradient);
        painter.setPen(gradient_pen);
        painter.drawLine(line);
        painter.setPen(solid_line_pen);
      } else {
        painter.drawLine(line);
      }
    }

    const QLineF line(to_screen.map(QLineF(m_ptrSpline->lastVertex()->point(), m_nextVertexImagePos)));
    gradient.setStart(line.p1());
    gradient.setFinalStop(line.p2());
    gradient_pen.setBrush(gradient);
    painter.setPen(gradient_pen);
    painter.drawLine(line);
  }

  m_visualizer.drawVertex(painter, to_screen.map(m_nextVertexImagePos), m_visualizer.highlightBrightColor());
}  // ZoneCreationInteraction::onPaint

void ZoneCreationInteraction::onKeyPressEvent(QKeyEvent* event, InteractionState& interaction) {
  if (event->key() == Qt::Key_Escape) {
    makePeerPreceeder(*m_rContext.createDefaultInteraction());
    m_rContext.imageView().update();
    delete this;
    event->accept();
  }
}

void ZoneCreationInteraction::onMousePressEvent(QMouseEvent* event, InteractionState& interaction) {
  if (event->button() != Qt::LeftButton) {
    return;
  }

  if (!m_rectangularZoneType && (event->modifiers() == m_lassoModeModifiers)) {
    m_lassoMode = true;
  }

  event->accept();
}

void ZoneCreationInteraction::onMouseReleaseEvent(QMouseEvent* event, InteractionState& interaction) {
  if (event->button() != Qt::LeftButton) {
    return;
  }
  if (m_dragWatcher.haveSignificantDrag() && !m_lassoMode) {
    return;
  }
  m_lassoMode = false;

  const QTransform to_screen(m_rContext.imageView().imageToWidget());
  const QTransform from_screen(m_rContext.imageView().widgetToImage());
  const QPointF screen_mouse_pos(event->pos() + QPointF(0.5, 0.5));
  const QPointF image_mouse_pos(from_screen.map(screen_mouse_pos));

  if (m_rectangularZoneType) {
    if (m_nextVertexImagePos != m_ptrSpline->firstVertex()->point()) {
      QPointF first_point = m_ptrSpline->firstVertex()->point();

      m_ptrSpline.reset(new EditableSpline);

      m_ptrSpline->appendVertex(first_point);
      m_ptrSpline->appendVertex(m_nextVertexImagePos_mid1);
      m_ptrSpline->appendVertex(m_nextVertexImagePos);
      m_ptrSpline->appendVertex(m_nextVertexImagePos_mid2);

      m_ptrSpline->setBridged(true);
      m_rContext.zones().addZone(m_ptrSpline);
      m_rContext.zones().commit();
    }

    makePeerPreceeder(*m_rContext.createDefaultInteraction());
    m_rContext.imageView().update();
    delete this;
  } else {
    if (m_ptrSpline->hasAtLeastSegments(2) && (m_nextVertexImagePos == m_ptrSpline->firstVertex()->point())) {
      // Finishing the spline.  Bridging the first and the last points
      // will create another segment.
      m_ptrSpline->setBridged(true);
      m_rContext.zones().addZone(m_ptrSpline);
      m_rContext.zones().commit();

      makePeerPreceeder(*m_rContext.createDefaultInteraction());
      m_rContext.imageView().update();
      delete this;
    } else if (m_nextVertexImagePos == m_ptrSpline->lastVertex()->point()) {
      // Removing the last vertex.
      m_ptrSpline->lastVertex()->remove();
      if (!m_ptrSpline->firstVertex()) {
        // If it was the only vertex, cancelling spline creation.
        makePeerPreceeder(*m_rContext.createDefaultInteraction());
        m_rContext.imageView().update();
        delete this;
      }
    } else {
      // Adding a new vertex, provided we are not too close to the previous one.
      const Proximity prox(screen_mouse_pos, m_ptrSpline->lastVertex()->point());
      if (prox > interaction.proximityThreshold()) {
        m_ptrSpline->appendVertex(image_mouse_pos);
        updateStatusTip();
      }
    }
  }

  event->accept();
}  // ZoneCreationInteraction::onMouseReleaseEvent

void ZoneCreationInteraction::onMouseMoveEvent(QMouseEvent* event, InteractionState& interaction) {
  const QPointF screen_mouse_pos(event->pos() + QPointF(0.5, 0.5));
  const QTransform to_screen(m_rContext.imageView().imageToWidget());
  const QTransform from_screen(m_rContext.imageView().widgetToImage());

  m_nextVertexImagePos = from_screen.map(screen_mouse_pos);
  const QPointF first(to_screen.map(m_ptrSpline->firstVertex()->point()));
  const QPointF last(to_screen.map(m_ptrSpline->lastVertex()->point()));

  if (!m_rectangularZoneType) {
    if (event->modifiers() == Qt::ControlModifier) {
      m_rectangularZoneType = true;
      m_lassoMode = false;
      updateStatusTip();
    } else if (!m_lassoMode && (event->modifiers() == m_lassoModeModifiers) && (event->buttons() & Qt::LeftButton)) {
      m_lassoMode = true;
    }
  }

  if (Proximity(last, screen_mouse_pos) <= interaction.proximityThreshold()) {
    m_nextVertexImagePos = m_ptrSpline->lastVertex()->point();
  } else if (m_ptrSpline->hasAtLeastSegments(2) || m_rectangularZoneType) {
    if (Proximity(first, screen_mouse_pos) <= interaction.proximityThreshold()) {
      m_nextVertexImagePos = m_ptrSpline->firstVertex()->point();
      updateStatusTip();
    }
  }

  if (m_rectangularZoneType && (m_nextVertexImagePos != m_ptrSpline->firstVertex()->point())) {
    QPointF screen_mouse_pos_mid1;
    screen_mouse_pos_mid1.setX(first.x());
    screen_mouse_pos_mid1.setY(screen_mouse_pos.y());

    QPointF screen_mouse_pos_mid2;
    screen_mouse_pos_mid2.setX(screen_mouse_pos.x());
    screen_mouse_pos_mid2.setY(first.y());

    qreal dx = screen_mouse_pos.x() - first.x();
    qreal dy = screen_mouse_pos.y() - first.y();

    if (((dx > 0) && (dy > 0)) || ((dx < 0) && (dy < 0))) {
      m_nextVertexImagePos_mid1 = from_screen.map(screen_mouse_pos_mid1);
      m_nextVertexImagePos_mid2 = from_screen.map(screen_mouse_pos_mid2);
    } else {
      m_nextVertexImagePos_mid2 = from_screen.map(screen_mouse_pos_mid1);
      m_nextVertexImagePos_mid1 = from_screen.map(screen_mouse_pos_mid2);
    }
  }

  if (m_lassoMode) {
    Proximity min_distance = Proximity::fromDist(10);
    if ((Proximity(last, screen_mouse_pos) > min_distance) && (Proximity(first, screen_mouse_pos) > min_distance)) {
      m_ptrSpline->appendVertex(m_nextVertexImagePos);
    }
  }

  m_rContext.imageView().update();
}  // ZoneCreationInteraction::onMouseMoveEvent

void ZoneCreationInteraction::updateStatusTip() {
  QString tip;

  if (m_rectangularZoneType) {
    if (m_nextVertexImagePos != m_ptrSpline->firstVertex()->point()) {
      tip = tr("Click to finish this rectangular zone.  ESC to cancel.");
    }
  } else {
    if (m_ptrSpline->hasAtLeastSegments(2)) {
      if (m_nextVertexImagePos == m_ptrSpline->firstVertex()->point()) {
        tip = tr("Click to finish this zone.  ESC to cancel.");
      } else {
        tip = tr("Connect first and last points to finish this zone.  ESC to cancel.");
      }
    } else {
      tip = tr("Hold Ctrl to create a rectangular zone or Shift+Alt+LMB to use lasso mode.  ESC to cancel.");
    }
  }

  m_interaction.setInteractionStatusTip(tip);
}

bool ZoneCreationInteraction::isDragHandlerPermitted(const InteractionState& interaction) const {
  return !m_lassoMode;
}
