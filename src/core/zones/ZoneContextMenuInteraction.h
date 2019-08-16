// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef ZONE_CONTEXT_MENU_INTERACTION_H_
#define ZONE_CONTEXT_MENU_INTERACTION_H_

#include <QColor>
#include <QObject>
#include <QtGlobal>
#include <boost/function.hpp>
#include <map>
#include <memory>
#include <vector>
#include "BasicSplineVisualizer.h"
#include "EditableSpline.h"
#include "EditableZoneSet.h"
#include "InteractionHandler.h"
#include "InteractionState.h"
#include "PropertySet.h"
#include "ZoneContextMenuItem.h"
#include "intrusive_ptr.h"

class ZoneInteractionContext;
class QPainter;
class QMenu;

class ZoneContextMenuInteraction : public QObject, public InteractionHandler {
  Q_OBJECT
 public:
  struct StandardMenuItems {
    ZoneContextMenuItem propertiesItem;
    ZoneContextMenuItem deleteItem;

    StandardMenuItems(const ZoneContextMenuItem& properties_item, const ZoneContextMenuItem& delete_item);
  };

  typedef boost::function<std::vector<ZoneContextMenuItem>(const EditableZoneSet::Zone&, const StandardMenuItems&)>
      MenuCustomizer;

  /**
   * \note This factory method will return null if there are no zones
   *       under the mouse pointer.
   */
  static ZoneContextMenuInteraction* create(ZoneInteractionContext& context, InteractionState& interaction);

  /**
   * Same as above, plus a menu customization callback.
   */
  static ZoneContextMenuInteraction* create(ZoneInteractionContext& context,
                                            InteractionState& interaction,
                                            const MenuCustomizer& menu_customizer);

  ~ZoneContextMenuInteraction() override;

 protected:
  class Zone : public EditableZoneSet::Zone {
   public:
    QColor color;

    explicit Zone(const EditableZoneSet::Zone& zone) : EditableZoneSet::Zone(zone) {}
  };


  static std::vector<Zone> zonesUnderMouse(ZoneInteractionContext& context);

  ZoneContextMenuInteraction(ZoneInteractionContext& context,
                             InteractionState& interaction,
                             const MenuCustomizer& menu_customizer,
                             std::vector<Zone>& selectable_zones);

  ZoneInteractionContext& context() { return m_context; }

 private slots:

  void menuAboutToHide();

  void highlightItem(int zone_idx);

 private:
  class OrderByArea;

  class Visualizer : public BasicSplineVisualizer {
   public:
    void switchToFillMode(const QColor& color);

    void switchToStrokeMode();

    void prepareForSpline(QPainter& painter, const EditableSpline::Ptr& spline) override;

   private:
    QColor m_color;
  };


  static std::vector<ZoneContextMenuItem> defaultMenuCustomizer(const EditableZoneSet::Zone& zone,
                                                                const StandardMenuItems& std_items);

  void onPaint(QPainter& painter, const InteractionState& interaction) override;

  void menuItemTriggered(InteractionState& interaction, const ZoneContextMenuItem::Callback& callback);

  InteractionHandler* deleteRequest(const EditableZoneSet::Zone& zone);

  InteractionHandler* propertiesRequest(const EditableZoneSet::Zone& zone);

  ZoneContextMenuItem propertiesMenuItemFor(const EditableZoneSet::Zone& zone);

  ZoneContextMenuItem deleteMenuItemFor(const EditableZoneSet::Zone& zone);

  ZoneInteractionContext& m_context;
  std::vector<Zone> m_selectableZones;
  InteractionState::Captor m_interaction;
  Visualizer m_visualizer;
  std::unique_ptr<QMenu> m_menu;
  int m_highlightedZoneIdx;
  bool m_menuItemTriggered;
#ifdef Q_OS_MAC
  int m_extraDelaysDone;
#endif
};


#endif  // ifndef ZONE_CONTEXT_MENU_INTERACTION_H_
