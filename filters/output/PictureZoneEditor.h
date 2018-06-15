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


#ifndef OUTPUT_PICTURE_ZONE_EDITOR_H_
#define OUTPUT_PICTURE_ZONE_EDITOR_H_

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
#include "imageproc/BinaryImage.h"
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
                    const ImagePixmapUnion& downscaled_image,
                    const imageproc::BinaryImage& picture_mask,
                    const QTransform& image_to_virt,
                    const QPolygonF& virt_display_area,
                    const PageId& page_id,
                    intrusive_ptr<Settings> settings);

  ~PictureZoneEditor() override;

 signals:

  void invalidateThumbnail(const PageId& page_id);

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
  intrusive_ptr<MaskTransformTask> m_ptrMaskTransformTask;

  PageId m_pageId;
  intrusive_ptr<Settings> m_ptrSettings;
};
}  // namespace output
#endif  // ifndef OUTPUT_PICTURE_ZONE_EDITOR_H_
