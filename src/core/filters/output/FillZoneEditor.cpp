// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "FillZoneEditor.h"

#include <QPainter>
#include <QPointer>
#include <boost/bind/bind.hpp>
#include <utility>

#include "ImagePresentation.h"
#include "ImageTransformation.h"
#include "OutputMargins.h"
#include "Settings.h"
#include "Zone.h"
#include "ZoneContextMenuInteraction.h"
#include "ZoneSet.h"

namespace output {
class FillZoneEditor::MenuCustomizer {
 private:
  using StdMenuItems = ZoneContextMenuInteraction::StandardMenuItems;

 public:
  explicit MenuCustomizer(FillZoneEditor* editor) : m_editor(editor) {}

  std::vector<ZoneContextMenuItem> operator()(const EditableZoneSet::Zone& zone, const StdMenuItems& stdItems);

 private:
  FillZoneEditor* m_editor;
};


FillZoneEditor::FillZoneEditor(const QImage& image,
                               const ImagePixmapUnion& downscaledVersion,
                               const boost::function<QPointF(const QPointF&)>& origToImage,
                               const boost::function<QPointF(const QPointF&)>& imageToOrig,
                               const PageId& pageId,
                               std::shared_ptr<Settings> settings)
    : ZoneEditorBase(image, downscaledVersion, ImagePresentation(QTransform(), QRectF(image.rect())), OutputMargins()),
      m_dragHandler(*this),
      m_zoomHandler(*this),
      m_colorAdapter(colorAdapterFor(image)),
      m_colorPickupInteraction(zones(), context()),
      m_origToImage(origToImage),
      m_imageToOrig(imageToOrig),
      m_pageId(pageId),
      m_settings(std::move(settings)) {
  zones().setDefaultProperties(m_settings->defaultFillZoneProperties());

  setMouseTracking(true);

  context().setContextMenuInteractionCreator(
      boost::bind(&FillZoneEditor::createContextMenuInteraction, this, boost::placeholders::_1));

  connect(&zones(), SIGNAL(committed()), SLOT(commitZones()));

  makeLastFollower(*context().createDefaultInteraction());

  rootInteractionHandler().makeLastFollower(*this);

  // We want these handlers after zone interaction handlers,
  // as some of those have their own drag and zoom handlers,
  // which need to get events before these standard ones.
  rootInteractionHandler().makeLastFollower(m_dragHandler);
  rootInteractionHandler().makeLastFollower(m_zoomHandler);

  for (const Zone& zone : m_settings->fillZonesForPage(pageId)) {
    auto spline = std::make_shared<EditableSpline>(zone.spline().transformed(m_origToImage));
    zones().addZone(spline, zone.properties());
  }
}

FillZoneEditor::~FillZoneEditor() {
  m_settings->setDefaultFillZoneProperties(zones().defaultProperties());
}

void FillZoneEditor::onPaint(QPainter& painter, const InteractionState& interaction) {
  if (m_colorPickupInteraction.isActive(interaction)) {
    return;
  }

  painter.setRenderHint(QPainter::Antialiasing, false);

  painter.setPen(Qt::NoPen);

  painter.setCompositionMode(QPainter::CompositionMode_SourceOver);

  for (const EditableZoneSet::Zone& zone : zones()) {
    using FCP = FillColorProperty;
    const QColor color(zone.properties()->locateOrDefault<FCP>()->color());
    painter.setBrush(m_colorAdapter(color));
    painter.drawPolygon(zone.spline()->toPolygon(), Qt::WindingFill);
  }
}

InteractionHandler* FillZoneEditor::createContextMenuInteraction(InteractionState& interaction) {
  // Return a standard ZoneContextMenuInteraction but with a customized menu.
  return ZoneContextMenuInteraction::create(context(), interaction, MenuCustomizer(this));
}

InteractionHandler* FillZoneEditor::createColorPickupInteraction(const EditableZoneSet::Zone& zone,
                                                                 InteractionState& interaction) {
  m_colorPickupInteraction.startInteraction(zone, interaction);
  return &m_colorPickupInteraction;
}

void FillZoneEditor::commitZones() {
  ZoneSet zones;

  for (const EditableZoneSet::Zone& zone : this->zones()) {
    const SerializableSpline spline = SerializableSpline(*zone.spline()).transformed(m_imageToOrig);
    zones.add(Zone(spline, *zone.properties()));
  }

  m_settings->setFillZones(m_pageId, zones);

  emit invalidateThumbnail(m_pageId);
}

void FillZoneEditor::updateRequested() {
  update();
}

QColor FillZoneEditor::toOpaque(const QColor& color) {
  QColor adapted(color);
  adapted.setAlpha(0xff);
  return adapted;
}

QColor FillZoneEditor::toGrayscale(const QColor& color) {
  const int gray = qGray(color.rgb());
  return QColor(gray, gray, gray);
}

QColor FillZoneEditor::toBlackWhite(const QColor& color) {
  const int gray = qGray(color.rgb());
  return gray < 128 ? Qt::black : Qt::white;
}

FillZoneEditor::ColorAdapter FillZoneEditor::colorAdapterFor(const QImage& image) {
  switch (image.format()) {
    case QImage::Format_Mono:
    case QImage::Format_MonoLSB:
      return &FillZoneEditor::toBlackWhite;
    case QImage::Format_Indexed8:
      if (image.allGray()) {
        return &FillZoneEditor::toGrayscale;
      }
      // fall through
    default:
      return &FillZoneEditor::toOpaque;
  }
}

/*=========================== MenuCustomizer =========================*/

std::vector<ZoneContextMenuItem> FillZoneEditor::MenuCustomizer::operator()(const EditableZoneSet::Zone& zone,
                                                                            const StdMenuItems& stdItems) {
  std::vector<ZoneContextMenuItem> items;
  items.reserve(2);
  items.emplace_back(tr("Pick color"), boost::bind(&FillZoneEditor::createColorPickupInteraction, m_editor, zone,
                                                   boost::placeholders::_1));
  items.push_back(stdItems.deleteItem);
  return items;
}
}  // namespace output
