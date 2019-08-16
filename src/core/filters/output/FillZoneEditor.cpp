// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "FillZoneEditor.h"
#include <QPainter>
#include <QPointer>
#include <boost/bind.hpp>
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
  typedef ZoneContextMenuInteraction::StandardMenuItems StdMenuItems;

 public:
  explicit MenuCustomizer(FillZoneEditor* editor) : m_editor(editor) {}

  std::vector<ZoneContextMenuItem> operator()(const EditableZoneSet::Zone& zone, const StdMenuItems& std_items);

 private:
  FillZoneEditor* m_editor;
};


FillZoneEditor::FillZoneEditor(const QImage& image,
                               const ImagePixmapUnion& downscaled_version,
                               const boost::function<QPointF(const QPointF&)>& orig_to_image,
                               const boost::function<QPointF(const QPointF&)>& image_to_orig,
                               const PageId& page_id,
                               intrusive_ptr<Settings> settings)
    : ImageViewBase(image, downscaled_version, ImagePresentation(QTransform(), QRectF(image.rect())), OutputMargins()),
      m_colorAdapter(colorAdapterFor(image)),
      m_context(*this, m_zones),
      m_colorPickupInteraction(m_zones, m_context),
      m_dragHandler(*this),
      m_zoomHandler(*this),
      m_origToImage(orig_to_image),
      m_imageToOrig(image_to_orig),
      m_pageId(page_id),
      m_settings(std::move(settings)) {
  m_zones.setDefaultProperties(m_settings->defaultFillZoneProperties());

  setMouseTracking(true);

  m_context.setContextMenuInteractionCreator(boost::bind(&FillZoneEditor::createContextMenuInteraction, this, _1));

  connect(&m_zones, SIGNAL(committed()), SLOT(commitZones()));

  makeLastFollower(*m_context.createDefaultInteraction());

  rootInteractionHandler().makeLastFollower(*this);

  // We want these handlers after zone interaction handlers,
  // as some of those have their own drag and zoom handlers,
  // which need to get events before these standard ones.
  rootInteractionHandler().makeLastFollower(m_dragHandler);
  rootInteractionHandler().makeLastFollower(m_zoomHandler);

  for (const Zone& zone : m_settings->fillZonesForPage(page_id)) {
    auto spline = make_intrusive<EditableSpline>(zone.spline().transformed(m_origToImage));
    m_zones.addZone(spline, zone.properties());
  }
}

FillZoneEditor::~FillZoneEditor() {
  m_settings->setDefaultFillZoneProperties(m_zones.defaultProperties());
}

void FillZoneEditor::onPaint(QPainter& painter, const InteractionState& interaction) {
  if (m_colorPickupInteraction.isActive(interaction)) {
    return;
  }

  painter.setRenderHint(QPainter::Antialiasing, false);

  painter.setPen(Qt::NoPen);

  painter.setCompositionMode(QPainter::CompositionMode_SourceOver);

  for (const EditableZoneSet::Zone& zone : m_zones) {
    typedef FillColorProperty FCP;
    const QColor color(zone.properties()->locateOrDefault<FCP>()->color());
    painter.setBrush(m_colorAdapter(color));
    painter.drawPolygon(zone.spline()->toPolygon(), Qt::WindingFill);
  }
}

InteractionHandler* FillZoneEditor::createContextMenuInteraction(InteractionState& interaction) {
  // Return a standard ZoneContextMenuInteraction but with a customized menu.
  return ZoneContextMenuInteraction::create(m_context, interaction, MenuCustomizer(this));
}

InteractionHandler* FillZoneEditor::createColorPickupInteraction(const EditableZoneSet::Zone& zone,
                                                                 InteractionState& interaction) {
  m_colorPickupInteraction.startInteraction(zone, interaction);

  return &m_colorPickupInteraction;
}

void FillZoneEditor::commitZones() {
  ZoneSet zones;

  for (const EditableZoneSet::Zone& zone : m_zones) {
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
                                                                            const StdMenuItems& std_items) {
  std::vector<ZoneContextMenuItem> items;
  items.reserve(2);
  items.emplace_back(tr("Pick color"), boost::bind(&FillZoneEditor::createColorPickupInteraction, m_editor, zone, _1));
  items.push_back(std_items.deleteItem);

  return items;
}
}  // namespace output