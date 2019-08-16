// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef DRAG_HANDLER_H_
#define DRAG_HANDLER_H_

#define BOOST_SIGNALS_NAMESPACE signal

#include <QCoreApplication>
#include <QPoint>
#include <boost/function.hpp>
#include "InteractionHandler.h"
#include "InteractionState.h"

class ImageViewBase;

class DragHandler : public InteractionHandler {
  Q_DECLARE_TR_FUNCTIONS(DragHandler)
 public:
  explicit DragHandler(ImageViewBase& image_view);

  DragHandler(ImageViewBase& image_view,
              const boost::function<bool(const InteractionState&)>& explicit_interaction_permitter);

  bool isActive() const;

 protected:
  void onMousePressEvent(QMouseEvent* event, InteractionState& interaction) override;

  void onMouseReleaseEvent(QMouseEvent* event, InteractionState& interaction) override;

  void onMouseMoveEvent(QMouseEvent* event, InteractionState& interaction) override;

 private:
  void init();

  ImageViewBase& m_imageView;
  InteractionState::Captor m_interaction;
  QPoint m_lastMousePos;
  boost::function<bool(const InteractionState&)> m_interactionPermitter;
};


#endif  // ifndef DRAG_HANDLER_H_
