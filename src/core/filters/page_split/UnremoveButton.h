// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef UNREMOVE_BUTTON_H_
#define UNREMOVE_BUTTON_H_

#include <QCoreApplication>
#include <QPixmap>
#include <QPointF>
#include <boost/function.hpp>
#include "InteractionHandler.h"
#include "InteractionState.h"
#include "Proximity.h"

namespace page_split {
class UnremoveButton : public InteractionHandler {
  Q_DECLARE_TR_FUNCTIONS(page_split::UnremoveButton)
 public:
  typedef boost::function<QPointF()> PositionGetter;
  typedef boost::function<void()> ClickCallback;

  explicit UnremoveButton(const PositionGetter& position_getter);

  void setClickCallback(const ClickCallback& callback);

 protected:
  void onPaint(QPainter& painter, const InteractionState& interaction) override;

  void onProximityUpdate(const QPointF& screen_mouse_pos, InteractionState& interaction) override;

  void onMousePressEvent(QMouseEvent* event, InteractionState& interaction) override;

 private:
  static void noOp();

  PositionGetter m_positionGetter;
  ClickCallback m_clickCallback;
  InteractionState::Captor m_proximityInteraction;
  QPixmap m_defaultPixmap;
  QPixmap m_hoveredPixmap;
  bool m_wasHovered;
};
}  // namespace page_split
#endif  // ifndef UNREMOVE_BUTTON_H_
