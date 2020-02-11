// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "InteractionHandler.h"

#include <QKeyEvent>
#include <QPainter>
#include <boost/lambda/bind.hpp>
#include <boost/lambda/construct.hpp>

#include "InteractionState.h"

#define DISPATCH(list, call)                      \
  {                                               \
    HandlerList::iterator it(list->begin());      \
    const HandlerList::iterator end(list->end()); \
    while (it != end) {                           \
      (it++)->call;                               \
    }                                             \
  }

#define RETURN_IF_ACCEPTED(event) \
  {                               \
    if (event->isAccepted()) {    \
      return;                     \
    }                             \
  }

namespace {
class ScopedClearAcceptance {
  DECLARE_NON_COPYABLE(ScopedClearAcceptance)

 public:
  explicit ScopedClearAcceptance(QEvent* event);

  ~ScopedClearAcceptance();

 private:
  QEvent* m_event;
  bool m_wasAccepted;
};


ScopedClearAcceptance::ScopedClearAcceptance(QEvent* event) : m_event(event), m_wasAccepted(event->isAccepted()) {
  m_event->setAccepted(false);
}

ScopedClearAcceptance::~ScopedClearAcceptance() {
  if (m_wasAccepted) {
    m_event->setAccepted(true);
  }
}
}  // anonymous namespace

InteractionHandler::InteractionHandler()
    : m_preceeders(std::make_shared<HandlerList>()), m_followers(std::make_shared<HandlerList>()) {}

InteractionHandler::~InteractionHandler() {
  using namespace boost::lambda;
  m_preceeders->clear_and_dispose(bind(delete_ptr(), _1));
  m_followers->clear_and_dispose(bind(delete_ptr(), _1));
}

void InteractionHandler::paint(QPainter& painter, const InteractionState& interaction) {
  // Keep them alive in case this object gets destroyed.
  std::shared_ptr<HandlerList> preceeders(m_preceeders);
  std::shared_ptr<HandlerList> followers(m_followers);

  DISPATCH(preceeders, paint(painter, interaction));
  painter.save();
  onPaint(painter, interaction);
  painter.restore();
  DISPATCH(followers, paint(painter, interaction));
}

void InteractionHandler::proximityUpdate(const QPointF& screenMousePos, InteractionState& interaction) {
  // Keep them alive in case this object gets destroyed.
  std::shared_ptr<HandlerList> preceeders(m_preceeders);
  std::shared_ptr<HandlerList> followers(m_followers);

  DISPATCH(preceeders, proximityUpdate(screenMousePos, interaction));
  onProximityUpdate(screenMousePos, interaction);
  assert(!interaction.captured() && "onProximityUpdate() must not capture interaction");
  DISPATCH(followers, proximityUpdate(screenMousePos, interaction));
}

void InteractionHandler::keyPressEvent(QKeyEvent* event, InteractionState& interaction) {
  RETURN_IF_ACCEPTED(event);

  // Keep them alive in case this object gets destroyed.
  std::shared_ptr<HandlerList> preceeders(m_preceeders);
  std::shared_ptr<HandlerList> followers(m_followers);

  DISPATCH(preceeders, keyPressEvent(event, interaction));
  RETURN_IF_ACCEPTED(event);
  onKeyPressEvent(event, interaction);
  ScopedClearAcceptance guard(event);
  DISPATCH(followers, keyPressEvent(event, interaction));
}

void InteractionHandler::keyReleaseEvent(QKeyEvent* event, InteractionState& interaction) {
  RETURN_IF_ACCEPTED(event);
  // Keep them alive in case this object gets destroyed.
  std::shared_ptr<HandlerList> preceeders(m_preceeders);
  std::shared_ptr<HandlerList> followers(m_followers);

  DISPATCH(preceeders, keyReleaseEvent(event, interaction));
  RETURN_IF_ACCEPTED(event);
  onKeyReleaseEvent(event, interaction);
  ScopedClearAcceptance guard(event);
  DISPATCH(followers, keyReleaseEvent(event, interaction));
}

void InteractionHandler::mousePressEvent(QMouseEvent* event, InteractionState& interaction) {
  RETURN_IF_ACCEPTED(event);

  // Keep them alive in case this object gets destroyed.
  std::shared_ptr<HandlerList> preceeders(m_preceeders);
  std::shared_ptr<HandlerList> followers(m_followers);

  DISPATCH(preceeders, mousePressEvent(event, interaction));
  RETURN_IF_ACCEPTED(event);
  onMousePressEvent(event, interaction);
  ScopedClearAcceptance guard(event);
  DISPATCH(followers, mousePressEvent(event, interaction));
}

void InteractionHandler::mouseReleaseEvent(QMouseEvent* event, InteractionState& interaction) {
  RETURN_IF_ACCEPTED(event);
  // Keep them alive in case this object gets destroyed.
  std::shared_ptr<HandlerList> preceeders(m_preceeders);
  std::shared_ptr<HandlerList> followers(m_followers);

  DISPATCH(preceeders, mouseReleaseEvent(event, interaction));
  RETURN_IF_ACCEPTED(event);
  onMouseReleaseEvent(event, interaction);
  ScopedClearAcceptance guard(event);
  DISPATCH(followers, mouseReleaseEvent(event, interaction));
}

void InteractionHandler::mouseDoubleClickEvent(QMouseEvent* event, InteractionState& interaction) {
  RETURN_IF_ACCEPTED(event);
  // Keep them alive in case this object gets destroyed.
  std::shared_ptr<HandlerList> preceeders(m_preceeders);
  std::shared_ptr<HandlerList> followers(m_followers);

  DISPATCH(preceeders, mouseDoubleClickEvent(event, interaction));
  RETURN_IF_ACCEPTED(event);
  onMouseDoubleClickEvent(event, interaction);
  ScopedClearAcceptance guard(event);
  DISPATCH(followers, mouseDoubleClickEvent(event, interaction));
}

void InteractionHandler::mouseMoveEvent(QMouseEvent* event, InteractionState& interaction) {
  RETURN_IF_ACCEPTED(event);

  // Keep them alive in case this object gets destroyed.
  std::shared_ptr<HandlerList> preceeders(m_preceeders);
  std::shared_ptr<HandlerList> followers(m_followers);

  DISPATCH(preceeders, mouseMoveEvent(event, interaction));
  RETURN_IF_ACCEPTED(event);
  onMouseMoveEvent(event, interaction);
  ScopedClearAcceptance guard(event);
  DISPATCH(followers, mouseMoveEvent(event, interaction));
}

void InteractionHandler::wheelEvent(QWheelEvent* event, InteractionState& interaction) {
  RETURN_IF_ACCEPTED(event);

  // Keep them alive in case this object gets destroyed.
  std::shared_ptr<HandlerList> preceeders(m_preceeders);
  std::shared_ptr<HandlerList> followers(m_followers);

  DISPATCH(preceeders, wheelEvent(event, interaction));
  RETURN_IF_ACCEPTED(event);
  onWheelEvent(event, interaction);
  ScopedClearAcceptance guard(event);
  DISPATCH(followers, wheelEvent(event, interaction));
}

void InteractionHandler::contextMenuEvent(QContextMenuEvent* event, InteractionState& interaction) {
  RETURN_IF_ACCEPTED(event);
  // Keep them alive in case this object gets destroyed.
  std::shared_ptr<HandlerList> preceeders(m_preceeders);
  std::shared_ptr<HandlerList> followers(m_followers);

  DISPATCH(preceeders, contextMenuEvent(event, interaction));
  RETURN_IF_ACCEPTED(event);
  onContextMenuEvent(event, interaction);
  ScopedClearAcceptance guard(event);
  DISPATCH(followers, contextMenuEvent(event, interaction));
}

void InteractionHandler::makePeerPreceeder(InteractionHandler& handler) {
  handler.unlink();
  HandlerList::node_algorithms::link_before(this, &handler);
}

void InteractionHandler::makePeerFollower(InteractionHandler& handler) {
  handler.unlink();
  HandlerList::node_algorithms::link_after(this, &handler);
}

void InteractionHandler::makeFirstPreceeder(InteractionHandler& handler) {
  handler.unlink();
  m_preceeders->push_front(handler);
}

void InteractionHandler::makeLastPreceeder(InteractionHandler& handler) {
  handler.unlink();
  m_preceeders->push_back(handler);
}

void InteractionHandler::makeFirstFollower(InteractionHandler& handler) {
  handler.unlink();
  m_followers->push_front(handler);
}

void InteractionHandler::makeLastFollower(InteractionHandler& handler) {
  handler.unlink();
  m_followers->push_back(handler);
}

bool InteractionHandler::defaultInteractionPermitter(const InteractionState& interaction) {
  return !interaction.captured();
}
