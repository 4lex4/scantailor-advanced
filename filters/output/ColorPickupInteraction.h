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

#ifndef OUTPUT_COLOR_PICKUP_INTERACTION_H_
#define OUTPUT_COLOR_PICKUP_INTERACTION_H_

#include <QCoreApplication>
#include <QRect>
#include <cstdint>
#include "EditableZoneSet.h"
#include "FillColorProperty.h"
#include "InteractionHandler.h"
#include "InteractionState.h"
#include "intrusive_ptr.h"

class ZoneInteractionContext;

namespace output {
class ColorPickupInteraction : public InteractionHandler {
  Q_DECLARE_TR_FUNCTIONS(ColorPickupInteraction)
 public:
  ColorPickupInteraction(EditableZoneSet& zones, ZoneInteractionContext& context);

  void startInteraction(const EditableZoneSet::Zone& zone, InteractionState& interaction);

  bool isActive(const InteractionState& interaction) const;

 protected:
  void onPaint(QPainter& painter, const InteractionState& interaction) override;

  void onMousePressEvent(QMouseEvent* event, InteractionState& interaction) override;

  void onMouseMoveEvent(QMouseEvent* event, InteractionState& interaction) override;

  void onKeyPressEvent(QKeyEvent* event, InteractionState& interaction) override;

 private:
  void takeColor();

  QRect targetBoundingRect() const;

  void switchToDefaultInteraction();

  static uint32_t bitMixColor(uint32_t color);

  static uint32_t bitUnmixColor(uint32_t mixed);

  EditableZoneSet& m_rZones;
  ZoneInteractionContext& m_rContext;
  InteractionState::Captor m_interaction;
  intrusive_ptr<FillColorProperty> m_ptrFillColorProp;
  int m_dontDrawCircle;

  static const uint32_t m_sBitMixingLUT[3][256];
  static const uint32_t m_sBitUnmixingLUT[3][256];
};
}  // namespace output
#endif  // ifndef OUTPUT_COLOR_PICKUP_INTERACTION_H_
