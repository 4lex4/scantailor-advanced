// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.


#ifndef SCANTAILOR_OUTPUT_PICTUREZONEEDITOR_H_
#define SCANTAILOR_OUTPUT_PICTUREZONEEDITOR_H_

#include <BinaryImage.h>

#include <QPixmap>
#include <QPoint>
#include <QTimer>
#include <QTransform>

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

class ImageTransformation;
class InteractionState;
class QPainter;
class QMenu;

namespace output {
class Settings;


class PictureZoneEditor : public ImageViewBase, private InteractionHandler {
  Q_OBJECT
 public:
  PictureZoneEditor(const QImage& image,
                    const ImagePixmapUnion& downscaledImage,
                    const imageproc::BinaryImage& pictureMask,
                    const QTransform& imageToVirt,
                    const QPolygonF& virtDisplayArea,
                    const PageId& pageId,
                    intrusive_ptr<Settings> settings);

  ~PictureZoneEditor() override;

 signals:

  void invalidateThumbnail(const PageId& pageId);

 protected:
  void onPaint(QPainter& painter, const InteractionState& interaction) override;

 private slots:

  void advancePictureMaskAnimation();

  void initiateBuildingScreenPictureMask();

  void commitZones();

  void updateRequested();

 private:
  class MaskTransformTask;

  bool validateScreenPictureMask() const;

  void schedulePictureMaskRebuild();

  void screenPictureMaskBuilt(const QPoint& origin, const QImage& mask);

  void paintOverPictureMask(QPainter& painter);

  void showPropertiesDialog(const EditableZoneSet::Zone& zone);

  EditableZoneSet m_zones;

  // Must go after m_zones.
  ZoneInteractionContext m_context;

  DragHandler m_dragHandler;
  ZoomHandler m_zoomHandler;

  imageproc::BinaryImage m_origPictureMask;
  QPixmap m_screenPictureMask;
  QPoint m_screenPictureMaskOrigin;
  QTransform m_screenPictureMaskXform;
  QTransform m_potentialPictureMaskXform;
  QTimer m_pictureMaskRebuildTimer;
  QTimer m_pictureMaskAnimateTimer;
  int m_pictureMaskAnimationPhase;  // degrees
  intrusive_ptr<MaskTransformTask> m_maskTransformTask;

  PageId m_pageId;
  intrusive_ptr<Settings> m_settings;
};
}  // namespace output
#endif  // ifndef SCANTAILOR_OUTPUT_PICTUREZONEEDITOR_H_
