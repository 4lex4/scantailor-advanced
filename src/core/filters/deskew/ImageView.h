// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

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
