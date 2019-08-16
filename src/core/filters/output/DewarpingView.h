// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef OUTPUT_DEWARPING_VIEW_H_
#define OUTPUT_DEWARPING_VIEW_H_

#include <QPointF>
#include <QPolygonF>
#include <QRectF>
#include <QTransform>
#include <vector>
#include "DepthPerception.h"
#include "DewarpingOptions.h"
#include "DragHandler.h"
#include "ImagePixmapUnion.h"
#include "ImageViewBase.h"
#include "InteractionHandler.h"
#include "InteractiveXSpline.h"
#include "PageId.h"
#include "Settings.h"
#include "ZoomHandler.h"
#include "DistortionModel.h"

namespace output {
class DewarpingView : public ImageViewBase, protected InteractionHandler {
  Q_OBJECT
 public:
  DewarpingView(const QImage& image,
                const ImagePixmapUnion& downscaled_image,
                const QTransform& source_to_virt,
                const QPolygonF& virt_display_area,
                const QRectF& virt_content_rect,
                const PageId& page_id,
                DewarpingOptions dewarping_options,
                const dewarping::DistortionModel& distortion_model,
                const DepthPerception& depth_perception);

  ~DewarpingView() override;

 signals:

  void distortionModelChanged(const dewarping::DistortionModel& model);

 public slots:

  void depthPerceptionChanged(double val);

 protected:
  void onPaint(QPainter& painter, const InteractionState& interaction) override;

 private:
  static void initNewSpline(XSpline& spline,
                            const QPointF& p1,
                            const QPointF& p2,
                            const DewarpingOptions* dewarpingOptions = nullptr);

  static void fitSpline(XSpline& spline, const std::vector<QPointF>& polyline);

  void paintXSpline(QPainter& painter, const InteractionState& interaction, const InteractiveXSpline& ispline);

  void curveModified(int curve_idx);

  void dragFinished();

  QPointF sourceToWidget(const QPointF& pt) const;

  QPointF widgetToSource(const QPointF& pt) const;

  QPolygonF virtMarginArea(int margin_idx) const;

  PageId m_pageId;
  QPolygonF m_virtDisplayArea;
  DewarpingOptions m_dewarpingOptions;
  dewarping::DistortionModel m_distortionModel;
  DepthPerception m_depthPerception;
  InteractiveXSpline m_topSpline;
  InteractiveXSpline m_bottomSpline;
  DragHandler m_dragHandler;
  ZoomHandler m_zoomHandler;
};
}  // namespace output
#endif  // ifndef OUTPUT_DEWARPING_VIEW_H_
