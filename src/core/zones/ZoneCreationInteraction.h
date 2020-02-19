// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_ZONES_ZONECREATIONINTERACTION_H_
#define SCANTAILOR_ZONES_ZONECREATIONINTERACTION_H_

#include <QCoreApplication>
#include <QDateTime>
#include <QPointF>

#include "BasicSplineVisualizer.h"
#include "DragHandler.h"
#include "DragWatcher.h"
#include "EditableSpline.h"
#include "InteractionHandler.h"
#include "InteractionState.h"
#include "ZoneCreationMode.h"
#include "ZoomHandler.h"

class ZoneInteractionContext;
class QShortcut;

class ZoneCreationInteraction : public InteractionHandler {
  Q_DECLARE_TR_FUNCTIONS(ZoneCreationInteraction)
 public:
  ZoneCreationInteraction(ZoneInteractionContext& context, InteractionState& interaction);

 protected:
  ZoneInteractionContext& context() { return m_context; }

  void onPaint(QPainter& painter, const InteractionState& interaction) override;

  void onMousePressEvent(QMouseEvent* event, InteractionState& interaction) override;

  void onMouseReleaseEvent(QMouseEvent* event, InteractionState& interaction) override;

  void onMouseMoveEvent(QMouseEvent* event, InteractionState& interaction) override;

 private:
  void updateStatusTip();

  bool isDragHandlerPermitted(const InteractionState& interaction) const;

  ZoneCreationMode currentZoneCreationMode() const;

  void cancel();

  ZoneInteractionContext& m_context;

  /**
   * We have our own drag handler even though there is already a global one
   * for the purpose of being able to monitor it with DragWatcher.  Because
   * we capture a state in the constructor, it's guaranteed the global
   * drag handler will not be functioning until we release the state.
   */
  DragHandler m_dragHandler;

  /**
   * This must go after m_dragHandler, otherwise DragHandler's destructor
   * will try to destroy this object.
   */
  DragWatcher m_dragWatcher;

  /**
   * Because we hold an interaction state from constructor to destructor,
   * we have to have our own zoom handler with explicit interaction permission
   * if we want zoom to work.
   */
  ZoomHandler m_zoomHandler;

  BasicSplineVisualizer m_visualizer;
  InteractionState::Captor m_interaction;
  EditableSpline::Ptr m_spline;
  QPointF m_nextVertexImagePos;
  QPointF m_nextVertexImagePos_mid1;
  QPointF m_nextVertexImagePos_mid2;
  ZoneCreationMode m_initialZoneCreationMode;
  bool m_leftMouseButtonPressed;
  Qt::KeyboardModifiers m_mouseButtonModifiers;
  std::unique_ptr<QShortcut> m_cancelShortcut;
};


#endif  // ifndef SCANTAILOR_ZONES_ZONECREATIONINTERACTION_H_
