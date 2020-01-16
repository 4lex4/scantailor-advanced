// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_INTERACTION_DRAGWATCHER_H_
#define SCANTAILOR_INTERACTION_DRAGWATCHER_H_

#include <QDateTime>
#include <QPoint>

#include "InteractionHandler.h"

class DragHandler;
class InteractionState;
class QMouseEvent;

/**
 * Objects of this class are intended to be used as a descendants followers
 * of DragHandler objects.  The idea is to collect some statistics about
 * an ongoing drag and decide if the drag was significant or not, in which
 * case we could perform some other operation on mouse release.
 */
class DragWatcher : public InteractionHandler {
 public:
  explicit DragWatcher(DragHandler& dragHandler);

  bool haveSignificantDrag() const;

 protected:
  void onMousePressEvent(QMouseEvent* event, InteractionState& interaction) override;

  void onMouseMoveEvent(QMouseEvent* event, InteractionState& interaction) override;

 private:
  void updateState(QPoint mousePos);

  DragHandler& m_dragHandler;
  QDateTime m_dragStartTime;
  QPoint m_dragStartPos;
  int m_dragMaxSqDist;
  bool m_dragInProgress;
};


#endif  // ifndef SCANTAILOR_INTERACTION_DRAGWATCHER_H_
