/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2007-2009  Joseph Artsimovich <joseph_a@mail.ru>

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

#ifndef ZONE_INTERACTION_CONTEXT_H_
#define ZONE_INTERACTION_CONTEXT_H_

#include <boost/function.hpp>
#include "EditableSpline.h"
#include "EditableZoneSet.h"
#include "SplineVertex.h"

class InteractionHandler;
class InteractionState;
class ImageViewBase;
class EditableZoneSet;

class ZoneInteractionContext {
 public:
  typedef boost::function<InteractionHandler*()> DefaultInteractionCreator;

  typedef boost::function<InteractionHandler*(InteractionState& interaction)> ZoneCreationInteractionCreator;

  typedef boost::function<InteractionHandler*(InteractionState& interaction,
                                              const EditableSpline::Ptr& spline,
                                              const SplineVertex::Ptr& vertex)>
      VertexDragInteractionCreator;

  typedef boost::function<InteractionHandler*(InteractionState& interaction, const EditableSpline::Ptr& spline)>
      ZoneDragInteractionCreator;

  typedef boost::function<InteractionHandler*(InteractionState& interaction)> ContextMenuInteractionCreator;

  typedef boost::function<void(const EditableZoneSet::Zone& zone)> ShowPropertiesCommand;

  ZoneInteractionContext(ImageViewBase& image_view, EditableZoneSet& zones);

  virtual ~ZoneInteractionContext();

  ImageViewBase& imageView() { return m_rImageView; }

  EditableZoneSet& zones() { return m_rZones; }

  virtual InteractionHandler* createDefaultInteraction() { return m_defaultInteractionCreator(); }

  void setDefaultInteractionCreator(const DefaultInteractionCreator& creator) { m_defaultInteractionCreator = creator; }

  virtual InteractionHandler* createZoneCreationInteraction(InteractionState& interaction) {
    return m_zoneCreationInteractionCreator(interaction);
  }

  void setZoneCreationInteractionCreator(const ZoneCreationInteractionCreator& creator) {
    m_zoneCreationInteractionCreator = creator;
  }

  virtual InteractionHandler* createVertexDragInteraction(InteractionState& interaction,
                                                          const EditableSpline::Ptr& spline,
                                                          const SplineVertex::Ptr& vertex) {
    return m_vertexDragInteractionCreator(interaction, spline, vertex);
  }

  void setVertexDragInteractionCreator(const VertexDragInteractionCreator& creator) {
    m_vertexDragInteractionCreator = creator;
  }

  virtual InteractionHandler* createZoneDragInteraction(InteractionState& interaction,
                                                        const EditableSpline::Ptr& spline) {
    return m_zoneDragInteractionCreator(interaction, spline);
  }

  void setZoneDragInteractionCreator(const ZoneDragInteractionCreator& creator) {
    m_zoneDragInteractionCreator = creator;
  }

  /**
   * \note This function may refuse to create a context menu interaction by returning null.
   */
  virtual InteractionHandler* createContextMenuInteraction(InteractionState& interaction) {
    return m_contextMenuInteractionCreator(interaction);
  }

  void setContextMenuInteractionCreator(const ContextMenuInteractionCreator& creator) {
    m_contextMenuInteractionCreator = creator;
  }

  virtual void showPropertiesCommand(const EditableZoneSet::Zone& zone) { m_showPropertiesCommand(zone); }

  void setShowPropertiesCommand(const ShowPropertiesCommand& command) { m_showPropertiesCommand = command; }

 private:
  /**
   * Creates an instance of ZoneDefaultInteraction.
   */
  InteractionHandler* createStdDefaultInteraction();

  /**
   * Creates an instance of ZoneCreationInteraction.
   */
  InteractionHandler* createStdZoneCreationInteraction(InteractionState& interaction);

  /**
   * Creates an instance of ZoneVertexDragInteraction.
   */
  InteractionHandler* createStdVertexDragInteraction(InteractionState& interaction,
                                                     const EditableSpline::Ptr& spline,
                                                     const SplineVertex::Ptr& vertex);

  /**
   * Creates an instance of ZoneDragInteraction.
   */
  InteractionHandler* createStdZoneDragInteraction(InteractionState& interaction, const EditableSpline::Ptr& spline);

  /**
   * Creates an instance of ZoneContextMenuInteraction.  May return null.
   */
  InteractionHandler* createStdContextMenuInteraction(InteractionState& interaction);

  static void showPropertiesStub(const EditableZoneSet::Zone&) {}

  ImageViewBase& m_rImageView;
  EditableZoneSet& m_rZones;
  DefaultInteractionCreator m_defaultInteractionCreator;
  ZoneCreationInteractionCreator m_zoneCreationInteractionCreator;
  VertexDragInteractionCreator m_vertexDragInteractionCreator;
  ZoneDragInteractionCreator m_zoneDragInteractionCreator;
  ContextMenuInteractionCreator m_contextMenuInteractionCreator;
  ShowPropertiesCommand m_showPropertiesCommand;
};


#endif  // ifndef ZONE_INTERACTION_CONTEXT_H_
