// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.


#ifndef SCANTAILOR_OUTPUT_FILLZONEEDITOR_H_
#define SCANTAILOR_OUTPUT_FILLZONEEDITOR_H_

#include <QColor>
#include <QPoint>
#include <QPointF>
#include <boost/function.hpp>

#include "ColorPickupInteraction.h"
#include "DragHandler.h"
#include "EditableSpline.h"
#include "EditableZoneSet.h"
#include "ImagePixmapUnion.h"
#include "NonCopyable.h"
#include "PageId.h"
#include "ZoneEditorBase.h"
#include "ZoneInteractionContext.h"
#include "ZoomHandler.h"
#include "intrusive_ptr.h"
#include "ref_countable.h"

class InteractionState;
class QPainter;

namespace output {
class Settings;

class FillZoneEditor : public ZoneEditorBase {
  Q_OBJECT
 public:
  FillZoneEditor(const QImage& image,
                 const ImagePixmapUnion& downscaledVersion,
                 const boost::function<QPointF(const QPointF&)>& origToImage,
                 const boost::function<QPointF(const QPointF&)>& imageToOrig,
                 const PageId& pageId,
                 intrusive_ptr<Settings> settings);

  ~FillZoneEditor() override;

 signals:

  void invalidateThumbnail(const PageId& pageId);

 protected:
  void onPaint(QPainter& painter, const InteractionState& interaction) override;

 private slots:

  void commitZones();

  void updateRequested();

 private:
  class MenuCustomizer;

  using ColorAdapter = QColor (*)(const QColor&);

  InteractionHandler* createContextMenuInteraction(InteractionState& interaction);

  InteractionHandler* createColorPickupInteraction(const EditableZoneSet::Zone& zone, InteractionState& interaction);

  static QColor toOpaque(const QColor& color);

  static QColor toGrayscale(const QColor& color);

  static QColor toBlackWhite(const QColor& color);

  static ColorAdapter colorAdapterFor(const QImage& image);

  DragHandler m_dragHandler;
  ZoomHandler m_zoomHandler;

  ColorAdapter m_colorAdapter;
  ColorPickupInteraction m_colorPickupInteraction;

  boost::function<QPointF(const QPointF&)> m_origToImage;
  boost::function<QPointF(const QPointF&)> m_imageToOrig;
  PageId m_pageId;
  intrusive_ptr<Settings> m_settings;
};
}  // namespace output
#endif  // ifndef SCANTAILOR_OUTPUT_FILLZONEEDITOR_H_
