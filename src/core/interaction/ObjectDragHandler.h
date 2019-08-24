// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_INTERACTION_OBJECTDRAGHANDLER_H_
#define SCANTAILOR_INTERACTION_OBJECTDRAGHANDLER_H_

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

  void setObject(DraggableObject* obj) { m_obj = obj; }

  void setProximityCursor(const QCursor& cursor);

  void setInteractionCursor(const QCursor& cursor);

  void setProximityStatusTip(const QString& tip);

  void setInteractionStatusTip(const QString& tip);

  bool interactionInProgress(const InteractionState& interaction) const;

  bool proximityLeader(const InteractionState& interaction) const;

  void forceEnterDragState(InteractionState& interaction, QPoint widgetMousePos);

  void setKeyboardModifiers(const std::set<Qt::KeyboardModifiers>& modifiersSet);

 protected:
  void onPaint(QPainter& painter, const InteractionState& interaction) override;

  void onProximityUpdate(const QPointF& screenMousePos, InteractionState& interaction) override;

  void onMousePressEvent(QMouseEvent* event, InteractionState& interaction) override;

  void onMouseReleaseEvent(QMouseEvent* event, InteractionState& interaction) override;

  void onMouseMoveEvent(QMouseEvent* event, InteractionState& interaction) override;

  void onKeyPressEvent(QKeyEvent* event, InteractionState& interaction) override;

  void onKeyReleaseEvent(QKeyEvent* event, InteractionState& interaction) override;

 private:
  DraggableObject* m_obj;
  InteractionState::Captor m_interaction;
  std::set<Qt::KeyboardModifiers> m_keyboardModifiersSet;
  Qt::KeyboardModifiers m_activeKeyboardModifiers;
};


#endif  // ifndef SCANTAILOR_INTERACTION_OBJECTDRAGHANDLER_H_
