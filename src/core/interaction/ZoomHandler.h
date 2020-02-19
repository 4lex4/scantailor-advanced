// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_INTERACTION_ZOOMHANDLER_H_
#define SCANTAILOR_INTERACTION_ZOOMHANDLER_H_

#include <QCoreApplication>
#include <QPoint>
#include <boost/function.hpp>

#include "InteractionHandler.h"
#include "InteractionState.h"

class ImageViewBase;
class QShortcut;

class ZoomHandler : public InteractionHandler {
  Q_DECLARE_TR_FUNCTIONS(ZoomHandler)
 public:
  enum Focus { CENTER, CURSOR };

  explicit ZoomHandler(ImageViewBase& imageView);

  ZoomHandler(ImageViewBase& imageView,
              const boost::function<bool(const InteractionState&)>& explicitInteractionPermitter);

  ~ZoomHandler() override;

  Focus focus() const { return m_focus; }

  void setFocus(Focus focus) { m_focus = focus; }

 protected:
  void onMouseMoveEvent(QMouseEvent* event, InteractionState& interaction) override;

  void onWheelEvent(QWheelEvent* event, InteractionState& interaction) override;

 private:
  void zoom(double factor);

  ImageViewBase& m_imageView;
  boost::function<bool(const InteractionState&)> m_interactionPermitter;
  InteractionState::Captor m_interaction;
  Focus m_focus;
  QPointF m_virtualMousePos;
  std::unique_ptr<QShortcut> m_magnifyShortcut;
  std::unique_ptr<QShortcut> m_diminishShortcut;
};


#endif  // ifndef SCANTAILOR_INTERACTION_ZOOMHANDLER_H_
