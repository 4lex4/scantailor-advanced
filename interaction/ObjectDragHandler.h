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

#ifndef OBJECT_DRAG_HANDLER_H_
#define OBJECT_DRAG_HANDLER_H_

#include <QPointF>
#include <set>
#include "DraggableObject.h"
#include "InteractionHandler.h"
#include "InteractionState.h"
#include "NonCopyable.h"

class QPainter;
class QCursor;
class QString;

class ObjectDragHandler : public InteractionHandler {
  DECLARE_NON_COPYABLE(ObjectDragHandler)

 public:
  explicit ObjectDragHandler(DraggableObject* obj = nullptr);

  void setObject(DraggableObject* obj) { m_pObj = obj; }

  void setProximityCursor(const QCursor& cursor);

  void setInteractionCursor(const QCursor& cursor);

  void setProximityStatusTip(const QString& tip);

  void setInteractionStatusTip(const QString& tip);

  bool interactionInProgress(const InteractionState& interaction) const;

  bool proximityLeader(const InteractionState& interaction) const;

  void forceEnterDragState(InteractionState& interaction, QPoint widget_mouse_pos);

  void setKeyboardModifiers(const std::set<Qt::KeyboardModifiers>& modifiers_set);

 protected:
  void onPaint(QPainter& painter, const InteractionState& interaction) override;

  void onProximityUpdate(const QPointF& screen_mouse_pos, InteractionState& interaction) override;

  void onMousePressEvent(QMouseEvent* event, InteractionState& interaction) override;

  void onMouseReleaseEvent(QMouseEvent* event, InteractionState& interaction) override;

  void onMouseMoveEvent(QMouseEvent* event, InteractionState& interaction) override;

  void onKeyPressEvent(QKeyEvent* event, InteractionState& interaction) override;

  void onKeyReleaseEvent(QKeyEvent* event, InteractionState& interaction) override;

 private:
  DraggableObject* m_pObj;
  InteractionState::Captor m_interaction;
  std::set<Qt::KeyboardModifiers> m_keyboardModifiersSet;
  Qt::KeyboardModifiers m_activeKeyboardModifiers;
};


#endif  // ifndef OBJECT_DRAG_HANDLER_H_
