// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "DragWatcher.h"

#include <QDebug>
#include <QMouseEvent>

#include "DragHandler.h"

DragWatcher::DragWatcher(DragHandler& dragHandler)
    : m_dragHandler(dragHandler), m_dragMaxSqDist(0), m_dragInProgress(false) {}

bool DragWatcher::haveSignificantDrag() const {
  if (!m_dragInProgress) {
    return false;
  }

  const QDateTime now(QDateTime::currentDateTime());
  qint64 msecPassed = m_dragStartTime.time().msecsTo(now.time());
  if (msecPassed < 0) {
    msecPassed += 60 * 60 * 24;
  }

  const double distScore = std::sqrt((double) m_dragMaxSqDist) / 12.0;
  const double timeScore = msecPassed / 500.0;
  return distScore + timeScore >= 1.0;
}

void DragWatcher::onMousePressEvent(QMouseEvent* event, InteractionState&) {
  updateState(event->pos());
}

void DragWatcher::onMouseMoveEvent(QMouseEvent* event, InteractionState&) {
  updateState(event->pos());
}

void DragWatcher::updateState(const QPoint mousePos) {
  if (m_dragHandler.isActive()) {
    if (!m_dragInProgress) {
      m_dragStartTime = QDateTime::currentDateTime();
      m_dragStartPos = mousePos;
      m_dragMaxSqDist = 0;
    } else {
      const QPoint delta(mousePos - m_dragStartPos);
      const int sqdist = delta.x() * delta.x() + delta.y() * delta.y();
      if (sqdist > m_dragMaxSqDist) {
        m_dragMaxSqDist = sqdist;
      }
    }
    m_dragInProgress = true;
  } else {
    m_dragInProgress = false;
  }
}
