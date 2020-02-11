// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_INTERACTION_INTERACTIONHANDLER_H_
#define SCANTAILOR_INTERACTION_INTERACTIONHANDLER_H_

#include <boost/intrusive/list.hpp>
#include <memory>

#include "NonCopyable.h"

class InteractionState;
class QPainter;
class QKeyEvent;
class QMouseEvent;
class QWheelEvent;
class QContextMenuEvent;
class QPointF;

class InteractionHandler
    : public boost::intrusive::list_base_hook<boost::intrusive::link_mode<boost::intrusive::auto_unlink>> {
  DECLARE_NON_COPYABLE(InteractionHandler)

 public:
  InteractionHandler();

  virtual ~InteractionHandler();

  void paint(QPainter& painter, const InteractionState& interaction);

  void proximityUpdate(const QPointF& screenMousePos, InteractionState& interaction);

  void keyPressEvent(QKeyEvent* event, InteractionState& interaction);

  void keyReleaseEvent(QKeyEvent* event, InteractionState& interaction);

  void mousePressEvent(QMouseEvent* event, InteractionState& interaction);

  void mouseReleaseEvent(QMouseEvent* event, InteractionState& interaction);

  void mouseDoubleClickEvent(QMouseEvent* event, InteractionState& interaction);

  void mouseMoveEvent(QMouseEvent* event, InteractionState& interaction);

  void wheelEvent(QWheelEvent* event, InteractionState& interaction);

  void contextMenuEvent(QContextMenuEvent* event, InteractionState& interaction);

  void makePeerPreceeder(InteractionHandler& handler);

  void makePeerFollower(InteractionHandler& handler);

  void makeFirstPreceeder(InteractionHandler& handler);

  void makeLastPreceeder(InteractionHandler& handler);

  void makeFirstFollower(InteractionHandler& handler);

  void makeLastFollower(InteractionHandler& handler);

 protected:
  virtual void onPaint(QPainter& painter, const InteractionState& interaction) {}

  virtual void onProximityUpdate(const QPointF& screenMousePos, InteractionState& interaction) {}

  virtual void onKeyPressEvent(QKeyEvent* event, InteractionState& interaction) {}

  virtual void onKeyReleaseEvent(QKeyEvent* event, InteractionState& interaction) {}

  virtual void onMousePressEvent(QMouseEvent* event, InteractionState& interaction) {}

  virtual void onMouseReleaseEvent(QMouseEvent* event, InteractionState& interaction) {}

  virtual void onMouseDoubleClickEvent(QMouseEvent* event, InteractionState& interaction) {}

  virtual void onMouseMoveEvent(QMouseEvent* event, InteractionState& interaction) {}

  virtual void onWheelEvent(QWheelEvent* event, InteractionState& interaction) {}

  virtual void onContextMenuEvent(QContextMenuEvent* event, InteractionState& interaction) {}

  static bool defaultInteractionPermitter(const InteractionState& interaction);

 private:
  class HandlerList : public boost::intrusive::list<InteractionHandler, boost::intrusive::constant_time_size<false>> {};


  std::shared_ptr<HandlerList> m_preceeders;
  std::shared_ptr<HandlerList> m_followers;
};


#endif  // ifndef SCANTAILOR_INTERACTION_INTERACTIONHANDLER_H_
