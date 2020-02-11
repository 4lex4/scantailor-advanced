// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "ZoneCreationInteraction.h"

#include <QDebug>
#include <QKeyEvent>
#include <QPainter>
#include <boost/bind.hpp>
#include <boost/lambda/lambda.hpp>

#include "ImageViewBase.h"
#include "ZoneInteractionContext.h"

ZoneCreationInteraction::ZoneCreationInteraction(ZoneInteractionContext& context, InteractionState& interaction)
    : m_context(context),
      m_dragHandler(context.imageView(), boost::bind(&ZoneCreationInteraction::isDragHandlerPermitted, this, _1)),
      m_dragWatcher(m_dragHandler),
      m_zoomHandler(context.imageView(), boost::lambda::constant(true)),
      m_spline(std::make_shared<EditableSpline>()),
      m_initialZoneCreationMode(context.getZoneCreationMode()),
      m_leftMouseButtonPressed(m_initialZoneCreationMode == ZoneCreationMode::LASSO) {
  const QPointF screenMousePos(m_context.imageView().mapFromGlobal(QCursor::pos()) + QPointF(0.5, 0.5));
  const QTransform fromScreen(m_context.imageView().widgetToImage());
  m_nextVertexImagePos = fromScreen.map(screenMousePos);

  m_nextVertexImagePos_mid1 = m_nextVertexImagePos;
  m_nextVertexImagePos_mid2 = m_nextVertexImagePos;

  makeLastFollower(m_dragHandler);
  m_dragHandler.makeFirstFollower(m_dragWatcher);

  makeLastFollower(m_zoomHandler);

  interaction.capture(m_interaction);
  m_spline->appendVertex(m_nextVertexImagePos);

  updateStatusTip();
}

void ZoneCreationInteraction::onPaint(QPainter& painter, const InteractionState& interaction) {
  painter.setWorldMatrixEnabled(false);
  painter.setRenderHint(QPainter::Antialiasing);

  const QTransform toScreen(m_context.imageView().imageToWidget());
  const QTransform fromScreen(m_context.imageView().widgetToImage());

  m_visualizer.drawSplines(painter, toScreen, m_context.zones());

  QPen solidLinePen(m_visualizer.solidColor());
  solidLinePen.setCosmetic(true);
  solidLinePen.setWidthF(1.5);

  QLinearGradient gradient;  // From inactive to active point.
  gradient.setColorAt(0.0, m_visualizer.solidColor());
  gradient.setColorAt(1.0, m_visualizer.highlightDarkColor());

  QPen gradientPen;
  gradientPen.setCosmetic(true);
  gradientPen.setWidthF(1.5);

  painter.setPen(solidLinePen);
  painter.setBrush(Qt::NoBrush);

  QColor startColor = m_visualizer.solidColor();
  QColor stopColor = m_visualizer.highlightDarkColor();
  QColor midColor;

  int red = (startColor.red() + stopColor.red()) / 2;
  int green = (startColor.green() + stopColor.green()) / 2;
  int blue = (startColor.blue() + stopColor.blue()) / 2;

  midColor.setRgb(red, green, blue);

  QLinearGradient gradientMid1;
  gradientMid1.setColorAt(0.0, startColor);
  gradientMid1.setColorAt(1.0, midColor);

  QLinearGradient gradientMid2;
  gradientMid2.setColorAt(0.0, midColor);
  gradientMid2.setColorAt(1.0, stopColor);

  if (currentZoneCreationMode() == ZoneCreationMode::RECTANGULAR) {
    if (m_nextVertexImagePos != m_spline->firstVertex()->point()) {
      m_visualizer.drawVertex(painter, toScreen.map(m_nextVertexImagePos_mid1), m_visualizer.highlightBrightColor());
      m_visualizer.drawVertex(painter, toScreen.map(m_nextVertexImagePos_mid2), m_visualizer.highlightBrightColor());

      const QLineF line1Mid1(toScreen.map(QLineF(m_spline->firstVertex()->point(), m_nextVertexImagePos_mid1)));
      gradientMid1.setStart(line1Mid1.p1());
      gradientMid1.setFinalStop(line1Mid1.p2());
      gradientPen.setBrush(gradientMid1);
      painter.setPen(gradientPen);
      painter.drawLine(line1Mid1);

      const QLineF line2Mid1(toScreen.map(QLineF(m_nextVertexImagePos_mid1, m_nextVertexImagePos)));
      gradientMid2.setStart(line2Mid1.p1());
      gradientMid2.setFinalStop(line2Mid1.p2());
      gradientPen.setBrush(gradientMid2);
      painter.setPen(gradientPen);
      painter.drawLine(line2Mid1);

      const QLineF line1Mid2(toScreen.map(QLineF(m_spline->firstVertex()->point(), m_nextVertexImagePos_mid2)));
      gradientMid1.setStart(line1Mid2.p1());
      gradientMid1.setFinalStop(line1Mid2.p2());
      gradientPen.setBrush(gradientMid1);
      painter.setPen(gradientPen);
      painter.drawLine(line1Mid2);

      const QLineF line2Mid2(toScreen.map(QLineF(m_nextVertexImagePos_mid2, m_nextVertexImagePos)));
      gradientMid2.setStart(line2Mid2.p1());
      gradientMid2.setFinalStop(line2Mid2.p2());
      gradientPen.setBrush(gradientMid2);
      painter.setPen(gradientPen);
      painter.drawLine(line2Mid2);
    }
  } else {
    for (EditableSpline::SegmentIterator it(*m_spline); it.hasNext();) {
      const SplineSegment segment(it.next());
      const QLineF line(toScreen.map(segment.toLine()));

      if ((segment.prev == m_spline->firstVertex()) && (segment.prev->point() == m_nextVertexImagePos)) {
        gradient.setStart(line.p2());
        gradient.setFinalStop(line.p1());
        gradientPen.setBrush(gradient);
        painter.setPen(gradientPen);
        painter.drawLine(line);
        painter.setPen(solidLinePen);
      } else {
        painter.drawLine(line);
      }
    }

    const QLineF line(toScreen.map(QLineF(m_spline->lastVertex()->point(), m_nextVertexImagePos)));
    gradient.setStart(line.p1());
    gradient.setFinalStop(line.p2());
    gradientPen.setBrush(gradient);
    painter.setPen(gradientPen);
    painter.drawLine(line);
  }

  m_visualizer.drawVertex(painter, toScreen.map(m_nextVertexImagePos), m_visualizer.highlightBrightColor());
}  // ZoneCreationInteraction::onPaint

void ZoneCreationInteraction::onKeyPressEvent(QKeyEvent* event, InteractionState& interaction) {
  if (event->key() == Qt::Key_Escape) {
    makePeerPreceeder(*m_context.createDefaultInteraction());
    m_context.imageView().update();
    delete this;
    event->accept();
  }
}

void ZoneCreationInteraction::onMousePressEvent(QMouseEvent* event, InteractionState& interaction) {
  if (event->button() != Qt::LeftButton) {
    return;
  }
  m_leftMouseButtonPressed = true;
  m_mouseButtonModifiers = event->modifiers();
  event->accept();
}

void ZoneCreationInteraction::onMouseReleaseEvent(QMouseEvent* event, InteractionState& interaction) {
  if (event->button() != Qt::LeftButton) {
    return;
  }
  m_leftMouseButtonPressed = false;

  ZoneCreationMode currentCreationMode = currentZoneCreationMode();
  if (m_dragWatcher.haveSignificantDrag()) {
    return;
  }

  const QTransform toScreen(m_context.imageView().imageToWidget());
  const QTransform fromScreen(m_context.imageView().widgetToImage());
  const QPointF screenMousePos(event->pos() + QPointF(0.5, 0.5));
  const QPointF imageMousePos(fromScreen.map(screenMousePos));

  if (currentCreationMode == ZoneCreationMode::RECTANGULAR) {
    if (m_nextVertexImagePos != m_spline->firstVertex()->point()) {
      QPointF firstPoint = m_spline->firstVertex()->point();

      m_spline.reset(new EditableSpline);

      m_spline->appendVertex(firstPoint);
      m_spline->appendVertex(m_nextVertexImagePos_mid1);
      m_spline->appendVertex(m_nextVertexImagePos);
      m_spline->appendVertex(m_nextVertexImagePos_mid2);

      m_spline->setBridged(true);
      m_context.zones().addZone(m_spline);
      m_context.zones().commit();
    }

    makePeerPreceeder(*m_context.createDefaultInteraction());
    m_context.imageView().update();
    delete this;
  } else {
    if (m_spline->hasAtLeastSegments(2) && (m_nextVertexImagePos == m_spline->firstVertex()->point())) {
      // Finishing the spline.  Bridging the first and the last points
      // will create another segment.
      m_spline->setBridged(true);
      m_context.zones().addZone(m_spline);
      m_context.zones().commit();

      makePeerPreceeder(*m_context.createDefaultInteraction());
      m_context.imageView().update();
      delete this;
    } else if (m_nextVertexImagePos == m_spline->lastVertex()->point()) {
      // Removing the last vertex.
      m_spline->lastVertex()->remove();
      if (!m_spline->firstVertex()) {
        // If it was the only vertex, cancelling spline creation.
        makePeerPreceeder(*m_context.createDefaultInteraction());
        m_context.imageView().update();
        delete this;
      }
    } else {
      // Adding a new vertex, provided we are not too close to the previous one.
      const Proximity prox(screenMousePos, m_spline->lastVertex()->point());
      if (prox > interaction.proximityThreshold()) {
        m_spline->appendVertex(imageMousePos);
        updateStatusTip();
      }
    }
  }

  event->accept();
}  // ZoneCreationInteraction::onMouseReleaseEvent

void ZoneCreationInteraction::onMouseMoveEvent(QMouseEvent* event, InteractionState& interaction) {
  const QPointF screenMousePos(event->pos() + QPointF(0.5, 0.5));
  const QTransform toScreen(m_context.imageView().imageToWidget());
  const QTransform fromScreen(m_context.imageView().widgetToImage());

  m_nextVertexImagePos = fromScreen.map(screenMousePos);
  const QPointF first(toScreen.map(m_spline->firstVertex()->point()));
  const QPointF last(toScreen.map(m_spline->lastVertex()->point()));

  ZoneCreationMode currentCreationMode = currentZoneCreationMode();

  if (Proximity(last, screenMousePos) <= interaction.proximityThreshold()) {
    m_nextVertexImagePos = m_spline->lastVertex()->point();
  } else if (m_spline->hasAtLeastSegments(2) || (currentCreationMode == ZoneCreationMode::RECTANGULAR)) {
    if (Proximity(first, screenMousePos) <= interaction.proximityThreshold()) {
      m_nextVertexImagePos = m_spline->firstVertex()->point();
      updateStatusTip();
    }
  }

  if ((currentCreationMode == ZoneCreationMode::RECTANGULAR)
      && (m_nextVertexImagePos != m_spline->firstVertex()->point())) {
    QPointF screenMousePosMid1;
    screenMousePosMid1.setX(first.x());
    screenMousePosMid1.setY(screenMousePos.y());

    QPointF screenMousePosMid2;
    screenMousePosMid2.setX(screenMousePos.x());
    screenMousePosMid2.setY(first.y());

    qreal dx = screenMousePos.x() - first.x();
    qreal dy = screenMousePos.y() - first.y();

    if (((dx > 0) && (dy > 0)) || ((dx < 0) && (dy < 0))) {
      m_nextVertexImagePos_mid1 = fromScreen.map(screenMousePosMid1);
      m_nextVertexImagePos_mid2 = fromScreen.map(screenMousePosMid2);
    } else {
      m_nextVertexImagePos_mid2 = fromScreen.map(screenMousePosMid1);
      m_nextVertexImagePos_mid1 = fromScreen.map(screenMousePosMid2);
    }
  }

  if ((currentCreationMode == ZoneCreationMode::LASSO) && m_leftMouseButtonPressed
      && (m_mouseButtonModifiers == Qt::NoModifier)) {
    Proximity minDistance = interaction.proximityThreshold();
    if ((Proximity(last, screenMousePos) > minDistance) && (Proximity(first, screenMousePos) > minDistance)) {
      m_spline->appendVertex(m_nextVertexImagePos);
    } else if (m_spline->hasAtLeastSegments(2) && (m_nextVertexImagePos == m_spline->firstVertex()->point())) {
      // Finishing the spline.  Bridging the first and the last points
      // will create another segment.
      m_spline->setBridged(true);
      m_context.zones().addZone(m_spline);
      m_context.zones().commit();

      makePeerPreceeder(*m_context.createDefaultInteraction());
      m_context.imageView().update();
      delete this;
    }
  }

  m_context.imageView().update();
}  // ZoneCreationInteraction::onMouseMoveEvent

void ZoneCreationInteraction::updateStatusTip() {
  QString tip;

  ZoneCreationMode currentCreationMode = currentZoneCreationMode();

  if (currentCreationMode == ZoneCreationMode::RECTANGULAR) {
    if (m_nextVertexImagePos != m_spline->firstVertex()->point()) {
      tip = tr("Click to finish this rectangular zone.  ESC to cancel.");
    }
  } else {
    if (m_spline->hasAtLeastSegments(2)) {
      if (m_nextVertexImagePos == m_spline->firstVertex()->point()) {
        tip = tr("Click to finish this zone.  ESC to cancel.");
      } else {
        tip = tr("Connect first and last points to finish this zone.  ESC to cancel.");
      }
    } else {
      tip = tr("Use Z and X keys to switch zone creation mode.  ESC to cancel.");
    }
  }

  m_interaction.setInteractionStatusTip(tip);
}

bool ZoneCreationInteraction::isDragHandlerPermitted(const InteractionState&) const {
  return !((currentZoneCreationMode() == ZoneCreationMode::LASSO) && m_leftMouseButtonPressed
           && (m_mouseButtonModifiers == Qt::NoModifier));
}

ZoneCreationMode ZoneCreationInteraction::currentZoneCreationMode() const {
  switch (m_initialZoneCreationMode) {
    case ZoneCreationMode::RECTANGULAR:
      return ZoneCreationMode::RECTANGULAR;
    case ZoneCreationMode::POLYGONAL:
    case ZoneCreationMode::LASSO:
      switch (m_context.getZoneCreationMode()) {
        case ZoneCreationMode::POLYGONAL:
          return ZoneCreationMode::POLYGONAL;
        case ZoneCreationMode::LASSO:
          return ZoneCreationMode::LASSO;
        default:
          break;
      }
  }
  return ZoneCreationMode::POLYGONAL;
}
