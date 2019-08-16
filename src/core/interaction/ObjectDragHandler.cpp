// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "ObjectDragHandler.h"
#include <QMouseEvent>

ObjectDragHandler::ObjectDragHandler(DraggableObject* obj)
    : m_obj(obj), m_keyboardModifiersSet({Qt::NoModifier}), m_activeKeyboardModifiers(Qt::NoModifier) {
  setProximityCursor(Qt::OpenHandCursor);
  setInteractionCursor(Qt::ClosedHandCursor);
}

void ObjectDragHandler::setProximityCursor(const QCursor& cursor) {
  m_interaction.setProximityCursor(cursor);
}

void ObjectDragHandler::setInteractionCursor(const QCursor& cursor) {
  m_interaction.setInteractionCursor(cursor);
}

void ObjectDragHandler::setProximityStatusTip(const QString& tip) {
  m_interaction.setProximityStatusTip(tip);
}

void ObjectDragHandler::setInteractionStatusTip(const QString& tip) {
  m_interaction.setInteractionStatusTip(tip);
}

bool ObjectDragHandler::interactionInProgress(const InteractionState& interaction) const {
  return interaction.capturedBy(m_interaction);
}

bool ObjectDragHandler::proximityLeader(const InteractionState& interaction) const {
  return interaction.proximityLeader(m_interaction);
}

void ObjectDragHandler::forceEnterDragState(InteractionState& interaction, QPoint widget_mouse_pos) {
  interaction.capture(m_interaction);
  m_obj->dragInitiated(QPointF(0.5, 0.5) + widget_mouse_pos);
}

void ObjectDragHandler::onPaint(QPainter& painter, const InteractionState& interaction) {
  m_obj->paint(painter, interaction);
}

void ObjectDragHandler::onProximityUpdate(const QPointF& screen_mouse_pos, InteractionState& interaction) {
  if (m_keyboardModifiersSet.find(m_activeKeyboardModifiers) == m_keyboardModifiersSet.end()) {
    return;
  }

  interaction.updateProximity(m_interaction, m_obj->proximity(screen_mouse_pos), m_obj->proximityPriority(),
                              m_obj->proximityThreshold(interaction));
}

void ObjectDragHandler::onMousePressEvent(QMouseEvent* event, InteractionState& interaction) {
  if (interaction.captured() || (event->button() != Qt::LeftButton)
      || (m_keyboardModifiersSet.find(m_activeKeyboardModifiers) == m_keyboardModifiersSet.end())) {
    return;
  }

  if (interaction.proximityLeader(m_interaction)) {
    interaction.capture(m_interaction);
    m_obj->dragInitiated(QPointF(0.5, 0.5) + event->pos());
  }
}

void ObjectDragHandler::onMouseReleaseEvent(QMouseEvent* event, InteractionState& interaction) {
  if ((event->button() == Qt::LeftButton) && interaction.capturedBy(m_interaction)) {
    m_interaction.release();
    m_obj->dragFinished(QPointF(0.5, 0.5) + event->pos());
  }
}

void ObjectDragHandler::onMouseMoveEvent(QMouseEvent* event, InteractionState& interaction) {
  if (interaction.capturedBy(m_interaction)) {
    m_obj->dragContinuation(QPointF(0.5, 0.5) + event->pos(), event->modifiers());
  }
}

void ObjectDragHandler::setKeyboardModifiers(const std::set<Qt::KeyboardModifiers>& modifiers_set) {
  m_keyboardModifiersSet = modifiers_set;
}

void ObjectDragHandler::onKeyPressEvent(QKeyEvent* event, InteractionState& interaction) {
  m_activeKeyboardModifiers = event->modifiers();
}

void ObjectDragHandler::onKeyReleaseEvent(QKeyEvent* event, InteractionState& interaction) {
  m_activeKeyboardModifiers = event->modifiers();
}
