// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "ZoneInteractionContext.h"

#include <boost/bind/bind.hpp>

#include "ZoneContextMenuInteraction.h"
#include "ZoneCreationInteraction.h"
#include "ZoneDefaultInteraction.h"
#include "ZoneDragInteraction.h"
#include "ZoneVertexDragInteraction.h"

ZoneInteractionContext::ZoneInteractionContext(ImageViewBase& imageView, EditableZoneSet& zones)
    : m_imageView(imageView),
      m_zones(zones),
      m_defaultInteractionCreator(boost::bind(&ZoneInteractionContext::createStdDefaultInteraction, this)),
      m_zoneCreationInteractionCreator(
          boost::bind(&ZoneInteractionContext::createStdZoneCreationInteraction, this,
                      boost::placeholders::_1)),
      m_vertexDragInteractionCreator(
          boost::bind(&ZoneInteractionContext::createStdVertexDragInteraction, this,
                      boost::placeholders::_1, boost::placeholders::_2, boost::placeholders::_3)),
      m_zoneDragInteractionCreator(boost::bind(&ZoneInteractionContext::createStdZoneDragInteraction,
                                               this, boost::placeholders::_1, boost::placeholders::_2)),
      m_contextMenuInteractionCreator(boost::bind(&ZoneInteractionContext::createStdContextMenuInteraction,
                                                  this, boost::placeholders::_1)),
      m_showPropertiesCommand(&ZoneInteractionContext::showPropertiesStub),
      m_zoneCreationMode(ZoneCreationMode::POLYGONAL) {}

ZoneInteractionContext::~ZoneInteractionContext() = default;

InteractionHandler* ZoneInteractionContext::createStdDefaultInteraction() {
  return new ZoneDefaultInteraction(*this);
}

InteractionHandler* ZoneInteractionContext::createStdZoneCreationInteraction(InteractionState& interaction) {
  return new ZoneCreationInteraction(*this, interaction);
}

InteractionHandler* ZoneInteractionContext::createStdVertexDragInteraction(InteractionState& interaction,
                                                                           const EditableSpline::Ptr& spline,
                                                                           const SplineVertex::Ptr& vertex) {
  return new ZoneVertexDragInteraction(*this, interaction, spline, vertex);
}

InteractionHandler* ZoneInteractionContext::createStdZoneDragInteraction(InteractionState& interaction,
                                                                         const EditableSpline::Ptr& spline) {
  return new ZoneDragInteraction(*this, interaction, spline);
}

InteractionHandler* ZoneInteractionContext::createStdContextMenuInteraction(InteractionState& interaction) {
  return ZoneContextMenuInteraction::create(*this, interaction);
}
