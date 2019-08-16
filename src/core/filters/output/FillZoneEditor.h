// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.


#ifndef OUTPUT_FILL_ZONE_EDITOR_H_
#define OUTPUT_FILL_ZONE_EDITOR_H_

#include <QColor>
#include <QPoint>
#include <QPointF>
#include <boost/function.hpp>
#include "ColorPickupInteraction.h"
#include "DragHandler.h"
#include "EditableSpline.h"
#include "EditableZoneSet.h"
#include "ImagePixmapUnion.h"
#include "ImageViewBase.h"
#include "NonCopyable.h"
#include "PageId.h"
#include "ZoneInteractionContext.h"
#include "ZoomHandler.h"
#include "intrusive_ptr.h"
#include "ref_countable.h"

class InteractionState;
class QPainter;

namespace output {
class Settings;

class FillZoneEditor : public ImageViewBase, private InteractionHandler {
  Q_OBJECT
 public:
  FillZoneEditor(const QImage& image,
                 const ImagePixmapUnion& downscaled_version,
                 const boost::function<QPointF(const QPointF&)>& orig_to_image,
                 const boost::function<QPointF(const QPointF&)>& image_to_orig,
                 const PageId& page_id,
                 intrusive_ptr<Settings> settings);

  ~FillZoneEditor() override;

 signals:

  void invalidateThumbnail(const PageId& page_id);

 protected:
  void onPaint(QPainter& painter, const InteractionState& interaction) override;

 private slots:

  void commitZones();

  void updateRequested();

 private:
  class MenuCustomizer;

  typedef QColor (*ColorAdapter)(const QColor&);

  InteractionHandler* createContextMenuInteraction(InteractionState& interaction);

  InteractionHandler* createColorPickupInteraction(const EditableZoneSet::Zone& zone, InteractionState& interaction);

  static QColor toOpaque(const QColor& color);

  static QColor toGrayscale(const QColor& color);

  static QColor toBlackWhite(const QColor& color);

  static ColorAdapter colorAdapterFor(const QImage& image);


  ColorAdapter m_colorAdapter;
  EditableZoneSet m_zones;

  // Must go after m_zones.
  ZoneInteractionContext m_context;
  // Must go after m_context.
  ColorPickupInteraction m_colorPickupInteraction;
  DragHandler m_dragHandler;
  ZoomHandler m_zoomHandler;

  boost::function<QPointF(const QPointF&)> m_origToImage;
  boost::function<QPointF(const QPointF&)> m_imageToOrig;
  PageId m_pageId;
  intrusive_ptr<Settings> m_settings;
};
}  // namespace output
#endif  // ifndef OUTPUT_FILL_ZONE_EDITOR_H_
