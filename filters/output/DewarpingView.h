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
#include "dewarping/DistortionModel.h"

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
                            const DewarpingOptions* p_dewarpingOptions = nullptr);

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
