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

#ifndef DESKEW_IMAGEVIEW_H_
#define DESKEW_IMAGEVIEW_H_

#include <QPixmap>
#include <QPoint>
#include <QPointF>
#include <QPolygonF>
#include <QRectF>
#include <QString>
#include <utility>
#include "DragHandler.h"
#include "DraggablePoint.h"
#include "ImageTransformation.h"
#include "ImageViewBase.h"
#include "ObjectDragHandler.h"
#include "ZoomHandler.h"

class QRect;

namespace deskew {
class ImageView : public ImageViewBase, private InteractionHandler {
  Q_OBJECT
 public:
  ImageView(const QImage& image, const QImage& downscaled_image, const ImageTransformation& xform);

  ~ImageView() override;

 signals:

  void manualDeskewAngleSet(double degrees);

 public slots:

  void manualDeskewAngleSetExternally(double degrees);

  void doRotateLeft();

  void doRotateRight();

 protected:
  void onPaint(QPainter& painter, const InteractionState& interaction) override;

  void onWheelEvent(QWheelEvent* event, InteractionState& interaction) override;

  void doRotate(double deg);

 private:
  QPointF handlePosition(int idx) const;

  void handleMoveRequest(int idx, const QPointF& pos);

  virtual void dragFinished();

  QPointF getImageRotationOrigin() const;

  QRectF getRotationArcSquare() const;

  std::pair<QPointF, QPointF> getRotationHandles(const QRectF& arc_square) const;

  static const int m_cellSize;
  static const double m_maxRotationDeg;
  static const double m_maxRotationSin;

  QPixmap m_handlePixmap;
  DraggablePoint m_handles[2];
  ObjectDragHandler m_handleInteractors[2];
  DragHandler m_dragHandler;
  ZoomHandler m_zoomHandler;
  ImageTransformation m_xform;
};
}  // namespace deskew
#endif  // ifndef DESKEW_IMAGEVIEW_H_
