/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C)  Joseph Artsimovich <joseph.artsimovich@gmail.com>

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
