// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "UnremoveButton.h"
#include <core/IconProvider.h>
#include <QMouseEvent>
#include <QPainter>

namespace page_split {
UnremoveButton::UnremoveButton(const PositionGetter& position_getter)
    : m_positionGetter(position_getter),
      m_clickCallback(&UnremoveButton::noOp),
      m_defaultPixmap(IconProvider::getInstance().getIcon("trashed").pixmap(128, 128)),
      m_hoveredPixmap(IconProvider::getInstance().getIcon("untrash").pixmap(128, 128)),
      m_wasHovered(false) {
  m_proximityInteraction.setProximityCursor(Qt::PointingHandCursor);
  m_proximityInteraction.setProximityStatusTip(tr("Restore removed page."));
}

void UnremoveButton::onPaint(QPainter& painter, const InteractionState& interaction) {
  const QPixmap& pixmap = interaction.proximityLeader(m_proximityInteraction) ? m_hoveredPixmap : m_defaultPixmap;

  QRectF rect(pixmap.rect());
  rect.moveCenter(m_positionGetter());

  painter.setWorldTransform(QTransform());
  painter.drawPixmap(rect.topLeft(), pixmap);
}

void UnremoveButton::onProximityUpdate(const QPointF& screen_mouse_pos, InteractionState& interaction) {
  QRectF rect(m_defaultPixmap.rect());
  rect.moveCenter(m_positionGetter());

  const bool hovered = rect.contains(screen_mouse_pos);
  if (hovered != m_wasHovered) {
    m_wasHovered = hovered;
    interaction.setRedrawRequested(true);
  }

  interaction.updateProximity(m_proximityInteraction, Proximity::fromSqDist(hovered ? 0.0 : 1e10));
}

void UnremoveButton::onMousePressEvent(QMouseEvent* event, InteractionState& interaction) {
  if (!interaction.captured() && interaction.proximityLeader(m_proximityInteraction)) {
    event->accept();
    m_clickCallback();
  }
}

void UnremoveButton::setClickCallback(const UnremoveButton::ClickCallback& callback) {
  m_clickCallback = callback;
}

void UnremoveButton::noOp() {}
}  // namespace page_split