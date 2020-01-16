// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "DragHandler.h"

#include <QMouseEvent>

#include "ImageViewBase.h"

DragHandler::DragHandler(ImageViewBase& imageView)
    : m_imageView(imageView), m_interactionPermitter(&InteractionHandler::defaultInteractionPermitter) {
  init();
}

DragHandler::DragHandler(ImageViewBase& imageView,
                         const boost::function<bool(const InteractionState&)>& explicitInteractionPermitter)
    : m_imageView(imageView), m_interactionPermitter(explicitInteractionPermitter) {
  init();
}

void DragHandler::init() {
  m_interaction.setInteractionStatusTip(tr("Unrestricted dragging is possible by holding down the Shift key."));
}

bool DragHandler::isActive() const {
  return m_imageView.interactionState().capturedBy(m_interaction);
}

void DragHandler::onMousePressEvent(QMouseEvent* event, InteractionState& interaction) {
  m_lastMousePos = event->pos();

  if ((event->buttons() & (Qt::LeftButton | Qt::MidButton)) && !interaction.capturedBy(m_interaction)
      && m_interactionPermitter(interaction)) {
    interaction.capture(m_interaction);
  }
}

void DragHandler::onMouseReleaseEvent(QMouseEvent* event, InteractionState& interaction) {
  if (!interaction.capturedBy(m_interaction)) {
    return;
  }

  m_interaction.release();
  event->accept();
}

void DragHandler::onMouseMoveEvent(QMouseEvent* event, InteractionState& interaction) {
  if (!interaction.capturedBy(m_interaction)) {
    return;
  }

  QPoint movement(event->pos());
  movement -= m_lastMousePos;
  m_lastMousePos = event->pos();

  QPointF adjustedFp(m_imageView.getWidgetFocalPoint());
  adjustedFp += movement;

  // These will call update() if necessary.
  if (event->modifiers() & Qt::ShiftModifier) {
    m_imageView.setWidgetFocalPoint(adjustedFp);
  } else {
    m_imageView.adjustAndSetWidgetFocalPoint(adjustedFp);
  }
}
