// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "ZoomHandler.h"
#include <QWheelEvent>
#include "ImageViewBase.h"

ZoomHandler::ZoomHandler(ImageViewBase& image_view)
    : m_imageView(image_view),
      m_interactionPermitter(&InteractionHandler::defaultInteractionPermitter),
      m_focus(CURSOR) {}

ZoomHandler::ZoomHandler(ImageViewBase& image_view,
                         const boost::function<bool(const InteractionState&)>& explicit_interaction_permitter)
    : m_imageView(image_view), m_interactionPermitter(explicit_interaction_permitter), m_focus(CURSOR) {}

void ZoomHandler::onWheelEvent(QWheelEvent* event, InteractionState& interaction) {
  if (event->orientation() != Qt::Vertical) {
    return;
  }

  if (!m_interactionPermitter(interaction)) {
    return;
  }

  event->accept();

  double zoom = m_imageView.zoomLevel();

  if ((zoom == 1.0) && (event->delta() < 0)) {
    // Alredy zoomed out and trying to zoom out more.

    // Scroll amount in terms of typical mouse wheel "clicks".
    const double delta_clicks = event->delta() / 120;

    const double dist = -delta_clicks * 30;  // 30px per "click"
    m_imageView.moveTowardsIdealPosition(dist);

    return;
  }

  const double degrees = event->delta() / 8.0;
  zoom *= std::pow(2.0, degrees / 60.0);  // 2 times zoom for every 60 degrees
  if (zoom < 1.0) {
    zoom = 1.0;
  }

  QPointF focus_point;
  switch (m_focus) {
    case CENTER:
      focus_point = QRectF(m_imageView.rect()).center();
      break;
    case CURSOR:
      focus_point = event->pos() + QPointF(0.5, 0.5);
      break;
  }
  m_imageView.setWidgetFocalPointWithoutMoving(focus_point);
  m_imageView.setZoomLevel(zoom);  // this will call update()
}  // ZoomHandler::onWheelEvent

void ZoomHandler::onKeyPressEvent(QKeyEvent* event, InteractionState& interaction) {
  if (!m_interactionPermitter(interaction)) {
    return;
  }

  double zoom = m_imageView.zoomLevel();

  switch (event->key()) {
    case Qt::Key_Plus:
      zoom *= 1.12246205;  // == 2^( 1/6);
      break;
    case Qt::Key_Minus:
      zoom *= 0.89089872;  // == 2^(-1/6);
      break;
    default:
      return;
  }

  QPointF focus_point = QRectF(m_imageView.rect()).center();

  m_imageView.setWidgetFocalPointWithoutMoving(focus_point);
  m_imageView.setZoomLevel(zoom);  // this will call update()
}
