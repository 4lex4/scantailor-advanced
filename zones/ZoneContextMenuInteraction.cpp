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

#include "ZoneContextMenuInteraction.h"
#include <QDebug>
#include <QMenu>
#include <QMessageBox>
#include <QPainter>
#include <QSignalMapper>
#include <boost/bind.hpp>
#include "ImageViewBase.h"
#include "QtSignalForwarder.h"
#include "ZoneInteractionContext.h"

class ZoneContextMenuInteraction::OrderByArea {
 public:
  bool operator()(const EditableZoneSet::Zone& lhs, const EditableZoneSet::Zone& rhs) const {
    const QRectF lhs_bbox(lhs.spline()->toPolygon().boundingRect());
    const QRectF rhs_bbox(rhs.spline()->toPolygon().boundingRect());
    const qreal lhs_area = lhs_bbox.width() * lhs_bbox.height();
    const qreal rhs_area = rhs_bbox.width() * rhs_bbox.height();

    return lhs_area < rhs_area;
  }
};


ZoneContextMenuInteraction* ZoneContextMenuInteraction::create(ZoneInteractionContext& context,
                                                               InteractionState& interaction) {
  return create(context, interaction, boost::bind(&ZoneContextMenuInteraction::defaultMenuCustomizer, _1, _2));
}

ZoneContextMenuInteraction* ZoneContextMenuInteraction::create(ZoneInteractionContext& context,
                                                               InteractionState& interaction,
                                                               const MenuCustomizer& menu_customizer) {
  std::vector<Zone> selectable_zones(zonesUnderMouse(context));

  if (selectable_zones.empty()) {
    return nullptr;
  } else {
    return new ZoneContextMenuInteraction(context, interaction, menu_customizer, selectable_zones);
  }
}

std::vector<ZoneContextMenuInteraction::Zone> ZoneContextMenuInteraction::zonesUnderMouse(
    ZoneInteractionContext& context) {
  const QTransform from_screen(context.imageView().widgetToImage());
  const QPointF image_mouse_pos(from_screen.map(context.imageView().mapFromGlobal(QCursor::pos()) + QPointF(0.5, 0.5)));

  // Find zones containing the mouse position.
  std::vector<Zone> selectable_zones;
  for (const EditableZoneSet::Zone& zone : context.zones()) {
    QPainterPath path;
    path.setFillRule(Qt::WindingFill);
    path.addPolygon(zone.spline()->toPolygon());
    if (path.contains(image_mouse_pos)) {
      selectable_zones.emplace_back(zone);
    }
  }

  return selectable_zones;
}

ZoneContextMenuInteraction::ZoneContextMenuInteraction(ZoneInteractionContext& context,
                                                       InteractionState& interaction,
                                                       const MenuCustomizer& menu_customizer,
                                                       std::vector<Zone>& selectable_zones)
    : m_rContext(context),
      m_ptrMenu(new QMenu(&context.imageView())),
      m_highlightedZoneIdx(-1),
      m_menuItemTriggered(false) {
#ifdef Q_OS_MAC
  m_extraDelaysDone = 0;
#endif

  m_selectableZones.swap(selectable_zones);
  std::sort(m_selectableZones.begin(), m_selectableZones.end(), OrderByArea());

  interaction.capture(m_interaction);

  int h = 20;
  const int h_step = 65;
  const int s = 255 * 64 / 100;
  const int v = 255 * 96 / 100;
  const int alpha = 150;
  QColor color;

  auto* hover_map = new QSignalMapper(this);
  connect(hover_map, SIGNAL(mapped(int)), SLOT(highlightItem(int)));

  QPixmap pixmap;

  auto it(m_selectableZones.begin());
  const auto end(m_selectableZones.end());
  for (int i = 0; it != end; ++it, ++i, h = (h + h_step) % 360) {
    color.setHsv(h, s, v, alpha);
    it->color = color.toRgb();

    if (m_selectableZones.size() > 1) {
      pixmap = QPixmap(16, 16);
      color.setAlpha(255);
      pixmap.fill(color);
    }

    const StandardMenuItems std_items(propertiesMenuItemFor(*it), deleteMenuItemFor(*it));

    for (const ZoneContextMenuItem& item : menu_customizer(*it, std_items)) {
      QAction* action = m_ptrMenu->addAction(pixmap, item.label());
      new QtSignalForwarder(
          action, SIGNAL(triggered()),
          boost::bind(&ZoneContextMenuInteraction::menuItemTriggered, this, boost::ref(interaction), item.callback()));

      hover_map->setMapping(action, i);
      connect(action, SIGNAL(hovered()), hover_map, SLOT(map()));
    }

    m_ptrMenu->addSeparator();
  }
  // The queued connection is used to ensure it gets called *after*
  // QAction::triggered().
  connect(m_ptrMenu.get(), SIGNAL(aboutToHide()), SLOT(menuAboutToHide()), Qt::QueuedConnection);

  highlightItem(0);
  m_ptrMenu->popup(QCursor::pos());
}

ZoneContextMenuInteraction::~ZoneContextMenuInteraction() = default;

void ZoneContextMenuInteraction::onPaint(QPainter& painter, const InteractionState&) {
  painter.setWorldMatrixEnabled(false);
  painter.setRenderHint(QPainter::Antialiasing);

  if (m_highlightedZoneIdx >= 0) {
    const QTransform to_screen(m_rContext.imageView().imageToWidget());
    const Zone& zone = m_selectableZones[m_highlightedZoneIdx];
    m_visualizer.drawSpline(painter, to_screen, zone.spline());
  }
}

void ZoneContextMenuInteraction::menuAboutToHide() {
  if (m_menuItemTriggered) {
    return;
  }

#ifdef Q_OS_MAC
  // On OSX, QAction::triggered() is emitted significantly (like 150ms)
  // later than QMenu::aboutToHide().  This makes it generally not possible
  // to tell whether the menu was just dismissed or a menu item was clicked.
  // The only way to tell is to check back later, which we do here.
  if (m_extraDelaysDone++ < 1) {
    QTimer::singleShot(200, this, SLOT(menuAboutToHide()));

    return;
  }
#endif

  InteractionHandler* next_handler = m_rContext.createDefaultInteraction();
  if (next_handler) {
    makePeerPreceeder(*next_handler);
  }

  unlink();
  m_rContext.imageView().update();
  deleteLater();
}

void ZoneContextMenuInteraction::menuItemTriggered(InteractionState& interaction,
                                                   const ZoneContextMenuItem::Callback& callback) {
  m_menuItemTriggered = true;
  m_visualizer.switchToStrokeMode();

  InteractionHandler* next_handler = callback(interaction);
  if (next_handler) {
    makePeerPreceeder(*next_handler);
  }

  unlink();
  m_rContext.imageView().update();
  deleteLater();
}

InteractionHandler* ZoneContextMenuInteraction::propertiesRequest(const EditableZoneSet::Zone& zone) {
  m_rContext.showPropertiesCommand(zone);

  return m_rContext.createDefaultInteraction();
}

InteractionHandler* ZoneContextMenuInteraction::deleteRequest(const EditableZoneSet::Zone& zone) {
  m_rContext.zones().removeZone(zone.spline());
  m_rContext.zones().commit();

  return m_rContext.createDefaultInteraction();
}

ZoneContextMenuItem ZoneContextMenuInteraction::deleteMenuItemFor(const EditableZoneSet::Zone& zone) {
  return ZoneContextMenuItem(tr("Delete"), boost::bind(&ZoneContextMenuInteraction::deleteRequest, this, zone));
}

ZoneContextMenuItem ZoneContextMenuInteraction::propertiesMenuItemFor(const EditableZoneSet::Zone& zone) {
  return ZoneContextMenuItem(tr("Properties"), boost::bind(&ZoneContextMenuInteraction::propertiesRequest, this, zone));
}

void ZoneContextMenuInteraction::highlightItem(const int zone_idx) {
  if (m_selectableZones.size() > 1) {
    m_visualizer.switchToFillMode(m_selectableZones[zone_idx].color);
  } else {
    m_visualizer.switchToStrokeMode();
  }
  m_highlightedZoneIdx = zone_idx;
  m_rContext.imageView().update();
}

std::vector<ZoneContextMenuItem> ZoneContextMenuInteraction::defaultMenuCustomizer(const EditableZoneSet::Zone& zone,
                                                                                   const StandardMenuItems& std_items) {
  std::vector<ZoneContextMenuItem> items;
  items.reserve(2);
  items.push_back(std_items.propertiesItem);
  items.push_back(std_items.deleteItem);

  return items;
}

/*========================== StandardMenuItem =========================*/

ZoneContextMenuInteraction::StandardMenuItems::StandardMenuItems(const ZoneContextMenuItem& properties_item,
                                                                 const ZoneContextMenuItem& delete_item)
    : propertiesItem(properties_item), deleteItem(delete_item) {}

/*============================= Visualizer ============================*/

void ZoneContextMenuInteraction::Visualizer::switchToFillMode(const QColor& color) {
  m_color = color;
}

void ZoneContextMenuInteraction::Visualizer::switchToStrokeMode() {
  m_color = QColor();
}

void ZoneContextMenuInteraction::Visualizer::prepareForSpline(QPainter& painter, const EditableSpline::Ptr& spline) {
  BasicSplineVisualizer::prepareForSpline(painter, spline);
  if (m_color.isValid()) {
    painter.setBrush(m_color);
  }
}
