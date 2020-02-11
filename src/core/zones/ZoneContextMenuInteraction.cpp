// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "ZoneContextMenuInteraction.h"

#include <QDebug>
#include <QMenu>
#include <QMessageBox>
#include <QPainter>
#include <boost/bind.hpp>

#include "ImageViewBase.h"
#include "ZoneInteractionContext.h"

class ZoneContextMenuInteraction::OrderByArea {
 public:
  bool operator()(const EditableZoneSet::Zone& lhs, const EditableZoneSet::Zone& rhs) const {
    const QRectF lhsBbox(lhs.spline()->toPolygon().boundingRect());
    const QRectF rhsBbox(rhs.spline()->toPolygon().boundingRect());
    const qreal lhsArea = lhsBbox.width() * lhsBbox.height();
    const qreal rhsArea = rhsBbox.width() * rhsBbox.height();
    return lhsArea < rhsArea;
  }
};


ZoneContextMenuInteraction* ZoneContextMenuInteraction::create(ZoneInteractionContext& context,
                                                               InteractionState& interaction) {
  return create(context, interaction, boost::bind(&ZoneContextMenuInteraction::defaultMenuCustomizer, _1, _2));
}

ZoneContextMenuInteraction* ZoneContextMenuInteraction::create(ZoneInteractionContext& context,
                                                               InteractionState& interaction,
                                                               const MenuCustomizer& menuCustomizer) {
  std::vector<Zone> selectableZones(zonesUnderMouse(context));

  if (selectableZones.empty()) {
    return nullptr;
  } else {
    return new ZoneContextMenuInteraction(context, interaction, menuCustomizer, selectableZones);
  }
}

std::vector<ZoneContextMenuInteraction::Zone> ZoneContextMenuInteraction::zonesUnderMouse(
    ZoneInteractionContext& context) {
  const QTransform fromScreen(context.imageView().widgetToImage());
  const QPointF imageMousePos(fromScreen.map(context.imageView().mapFromGlobal(QCursor::pos()) + QPointF(0.5, 0.5)));

  // Find zones containing the mouse position.
  std::vector<Zone> selectableZones;
  for (const EditableZoneSet::Zone& zone : context.zones()) {
    QPainterPath path;
    path.setFillRule(Qt::WindingFill);
    path.addPolygon(zone.spline()->toPolygon());
    if (path.contains(imageMousePos)) {
      selectableZones.emplace_back(zone);
    }
  }
  return selectableZones;
}

ZoneContextMenuInteraction::ZoneContextMenuInteraction(ZoneInteractionContext& context,
                                                       InteractionState& interaction,
                                                       const MenuCustomizer& menuCustomizer,
                                                       std::vector<Zone>& selectableZones)
    : m_context(context),
      m_menu(std::make_unique<QMenu>(&context.imageView())),
      m_highlightedZoneIdx(-1),
      m_menuItemTriggered(false) {
#ifdef Q_OS_MAC
  m_extraDelaysDone = 0;
#endif

  m_selectableZones.swap(selectableZones);
  std::sort(m_selectableZones.begin(), m_selectableZones.end(), OrderByArea());

  interaction.capture(m_interaction);

  int h = 20;
  const int hStep = 65;
  const int s = 255 * 64 / 100;
  const int v = 255 * 96 / 100;
  const int alpha = 150;
  QColor color;
  QPixmap pixmap;
  auto it(m_selectableZones.begin());
  const auto end(m_selectableZones.end());
  for (int i = 0; it != end; ++it, ++i, h = (h + hStep) % 360) {
    color.setHsv(h, s, v, alpha);
    it->color = color.toRgb();

    if (m_selectableZones.size() > 1) {
      pixmap = QPixmap(16, 16);
      color.setAlpha(255);
      pixmap.fill(color);
    }

    const StandardMenuItems stdItems(propertiesMenuItemFor(*it), deleteMenuItemFor(*it));

    for (const ZoneContextMenuItem& item : menuCustomizer(*it, stdItems)) {
      QAction* action = m_menu->addAction(pixmap, item.label());
      connect(
          action, &QAction::triggered,
          boost::bind(&ZoneContextMenuInteraction::menuItemTriggered, this, boost::ref(interaction), item.callback()));
      connect(action, &QAction::hovered, this, boost::bind(&ZoneContextMenuInteraction::highlightItem, this, i));
    }

    m_menu->addSeparator();
  }
  // The queued connection is used to ensure it gets called *after*
  // QAction::triggered().
  connect(m_menu.get(), SIGNAL(aboutToHide()), SLOT(menuAboutToHide()), Qt::QueuedConnection);

  highlightItem(0);
  m_menu->popup(QCursor::pos());
}

ZoneContextMenuInteraction::~ZoneContextMenuInteraction() = default;

void ZoneContextMenuInteraction::onPaint(QPainter& painter, const InteractionState&) {
  painter.setWorldMatrixEnabled(false);
  painter.setRenderHint(QPainter::Antialiasing);

  if (m_highlightedZoneIdx >= 0) {
    const QTransform toScreen(m_context.imageView().imageToWidget());
    const Zone& zone = m_selectableZones[m_highlightedZoneIdx];
    m_visualizer.drawSpline(painter, toScreen, zone.spline());
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

  InteractionHandler* nextHandler = m_context.createDefaultInteraction();
  if (nextHandler) {
    makePeerPreceeder(*nextHandler);
  }

  unlink();
  m_context.imageView().update();
  deleteLater();
}

void ZoneContextMenuInteraction::menuItemTriggered(InteractionState& interaction,
                                                   const ZoneContextMenuItem::Callback& callback) {
  m_menuItemTriggered = true;
  m_visualizer.switchToStrokeMode();

  InteractionHandler* nextHandler = callback(interaction);
  if (nextHandler) {
    makePeerPreceeder(*nextHandler);
  }

  unlink();
  m_context.imageView().update();
  deleteLater();
}

InteractionHandler* ZoneContextMenuInteraction::propertiesRequest(const EditableZoneSet::Zone& zone) {
  m_context.showPropertiesCommand(zone);
  return m_context.createDefaultInteraction();
}

InteractionHandler* ZoneContextMenuInteraction::deleteRequest(const EditableZoneSet::Zone& zone) {
  m_context.zones().removeZone(zone.spline());
  m_context.zones().commit();
  return m_context.createDefaultInteraction();
}

ZoneContextMenuItem ZoneContextMenuInteraction::deleteMenuItemFor(const EditableZoneSet::Zone& zone) {
  return ZoneContextMenuItem(tr("Delete"), boost::bind(&ZoneContextMenuInteraction::deleteRequest, this, zone));
}

ZoneContextMenuItem ZoneContextMenuInteraction::propertiesMenuItemFor(const EditableZoneSet::Zone& zone) {
  return ZoneContextMenuItem(tr("Properties"), boost::bind(&ZoneContextMenuInteraction::propertiesRequest, this, zone));
}

void ZoneContextMenuInteraction::highlightItem(const int zoneIdx) {
  if (m_selectableZones.size() > 1) {
    m_visualizer.switchToFillMode(m_selectableZones[zoneIdx].color);
  } else {
    m_visualizer.switchToStrokeMode();
  }
  m_highlightedZoneIdx = zoneIdx;
  m_context.imageView().update();
}

std::vector<ZoneContextMenuItem> ZoneContextMenuInteraction::defaultMenuCustomizer(const EditableZoneSet::Zone& zone,
                                                                                   const StandardMenuItems& stdItems) {
  std::vector<ZoneContextMenuItem> items;
  items.reserve(2);
  items.push_back(stdItems.propertiesItem);
  items.push_back(stdItems.deleteItem);
  return items;
}

/*========================== StandardMenuItem =========================*/

ZoneContextMenuInteraction::StandardMenuItems::StandardMenuItems(const ZoneContextMenuItem& propertiesItem,
                                                                 const ZoneContextMenuItem& deleteItem)
    : propertiesItem(propertiesItem), deleteItem(deleteItem) {}

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
