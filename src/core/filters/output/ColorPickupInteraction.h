// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

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

  EditableZoneSet& m_zones;
  ZoneInteractionContext& m_context;
  InteractionState::Captor m_interaction;
  intrusive_ptr<FillColorProperty> m_fillColorProp;
  int m_dontDrawCircle;

  static const uint32_t m_sBitMixingLUT[3][256];
  static const uint32_t m_sBitUnmixingLUT[3][256];
};
}  // namespace output
#endif  // ifndef OUTPUT_COLOR_PICKUP_INTERACTION_H_
