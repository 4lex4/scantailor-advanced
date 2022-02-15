// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "ZoomHandler.h"

#include <QWheelEvent>
#include <QShortcut>
#include <cmath>

#include "ImageViewBase.h"

ZoomHandler::ZoomHandler(ImageViewBase& imageView)
    : ZoomHandler(imageView, &InteractionHandler::defaultInteractionPermitter) {}

ZoomHandler::ZoomHandler(ImageViewBase& imageView,
                         const boost::function<bool(const InteractionState&)>& explicitInteractionPermitter)
    : m_imageView(imageView), m_interactionPermitter(explicitInteractionPermitter), m_focus(CURSOR) {
  m_magnifyShortcut = std::make_unique<QShortcut>(Qt::Key_Plus, &m_imageView);
  m_diminishShortcut = std::make_unique<QShortcut>(Qt::Key_Minus, &m_imageView);
  QObject::connect(m_magnifyShortcut.get(), &QShortcut::activated, &m_imageView,
                   std::bind(&ZoomHandler::zoom, this, std::pow(2, 1.0 / 6)));
  QObject::connect(m_diminishShortcut.get(), &QShortcut::activated, &m_imageView,
                   std::bind(&ZoomHandler::zoom, this, std::pow(2, -1.0 / 6)));
}

ZoomHandler::~ZoomHandler() = default;

void ZoomHandler::onWheelEvent(QWheelEvent* event, InteractionState& interaction) {
  if (!m_interactionPermitter(interaction)) {
    return;
  }

  event->accept();

  double zoom = m_imageView.zoomLevel();
  if ((zoom == 1.0) && (event->angleDelta().y() < 0)) {
    // Already zoomed out and trying to zoom out more.
    // Scroll amount in terms of typical mouse wheel "clicks".
    const double deltaClicks = event->angleDelta().y() / 120;
    const double dist = -deltaClicks * 30;  // 30px per "click"
    m_imageView.moveTowardsIdealPosition(dist);
    return;
  }
  const double degrees = event->angleDelta().y() / 8.0;
  zoom *= std::pow(2.0, degrees / 60.0);  // 2 times zoom for every 60 degrees
  if (zoom < 1.0) {
    zoom = 1.0;
  }

  QPointF focusPoint;
  switch (m_focus) {
    case CENTER:
      focusPoint = QRectF(m_imageView.rect()).center();
      break;
    case CURSOR:
      focusPoint = event->position() + QPointF(0.5, 0.5);
      break;
  }
  m_imageView.setWidgetFocalPointWithoutMoving(focusPoint);
  m_imageView.setZoomLevel(zoom);  // this will call update()
}  // ZoomHandler::onWheelEvent

void ZoomHandler::zoom(double factor) {
  if (!m_interactionPermitter(m_imageView.interactionState())) {
    return;
  }

  double zoom = m_imageView.zoomLevel();
  if ((zoom == 1.0) && (factor < 1)) {
    // Already zoomed out and trying to zoom out more.
    m_imageView.moveTowardsIdealPosition(20);
    return;
  }
  zoom *= factor;
  if (zoom < 1.0)
    zoom = 1.0;

  QPointF focusPoint;
  switch (m_focus) {
    case CENTER:
      focusPoint = QRectF(m_imageView.rect()).center();
      break;
    case CURSOR:
      focusPoint = m_virtualMousePos;
      break;
  }
  m_imageView.setWidgetFocalPointWithoutMoving(focusPoint);
  m_imageView.setZoomLevel(zoom);  // this will call update()
}

void ZoomHandler::onMouseMoveEvent(QMouseEvent* event, InteractionState&) {
  m_virtualMousePos = event->pos() + QPointF(0.5, 0.5);
}
