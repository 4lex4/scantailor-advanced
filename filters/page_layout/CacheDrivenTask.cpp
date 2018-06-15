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

#include "CacheDrivenTask.h"

#include <QtCore/QSettings>
#include <utility>
#include "IncompleteThumbnail.h"
#include "PageInfo.h"
#include "Params.h"
#include "Settings.h"
#include "Thumbnail.h"
#include "Utils.h"
#include "filter_dc/AbstractFilterDataCollector.h"
#include "filter_dc/ThumbnailCollector.h"
#include "filters/output/CacheDrivenTask.h"

namespace page_layout {
CacheDrivenTask::CacheDrivenTask(intrusive_ptr<output::CacheDrivenTask> next_task, intrusive_ptr<Settings> settings)
    : m_ptrNextTask(std::move(next_task)), m_ptrSettings(std::move(settings)) {}

CacheDrivenTask::~CacheDrivenTask() = default;

void CacheDrivenTask::process(const PageInfo& page_info,
                              AbstractFilterDataCollector* collector,
                              const ImageTransformation& xform,
                              const QRectF& page_rect,
                              const QRectF& content_rect) {
  const std::unique_ptr<Params> params(m_ptrSettings->getPageParams(page_info.id()));
  if (!params || (params->contentSizeMM().isEmpty() && !content_rect.isEmpty())) {
    if (auto* thumb_col = dynamic_cast<ThumbnailCollector*>(collector)) {
      thumb_col->processThumbnail(std::unique_ptr<QGraphicsItem>(new IncompleteThumbnail(
          thumb_col->thumbnailCache(), thumb_col->maxLogicalThumbSize(), page_info.imageId(), xform)));
    }

    return;
  }

  Params new_params(
      m_ptrSettings->updateContentSizeAndGetParams(page_info.id(), page_rect, content_rect, params->contentSizeMM()));

  const QRectF adapted_content_rect(Utils::adaptContentRect(xform, content_rect));
  const QPolygonF content_rect_phys(xform.transformBack().map(adapted_content_rect));
  const QPolygonF page_rect_phys(Utils::calcPageRectPhys(xform, content_rect_phys, new_params,
                                                         m_ptrSettings->getAggregateHardSizeMM(),
                                                         m_ptrSettings->getAggregateContentRect()));

  ImageTransformation new_xform(xform);
  new_xform.setPostCropArea(shiftToRoundedOrigin(new_xform.transform().map(page_rect_phys)));

  if (m_ptrNextTask) {
    m_ptrNextTask->process(page_info, collector, new_xform, content_rect_phys);

    return;
  }

  QSettings settings;
  const double deviationCoef = settings.value("settings/marginsDeviationCoef", 0.35).toDouble();
  const double deviationThreshold = settings.value("settings/marginsDeviationThreshold", 1.0).toDouble();

  if (auto* thumb_col = dynamic_cast<ThumbnailCollector*>(collector)) {
    thumb_col->processThumbnail(std::unique_ptr<QGraphicsItem>(new Thumbnail(
        thumb_col->thumbnailCache(), thumb_col->maxLogicalThumbSize(), page_info.imageId(), new_params, xform,
        content_rect_phys, xform.transform().map(page_rect_phys).boundingRect(),
        m_ptrSettings->deviationProvider().isDeviant(page_info.id(), deviationCoef, deviationThreshold))));
  }
}  // CacheDrivenTask::process

QPolygonF CacheDrivenTask::shiftToRoundedOrigin(const QPolygonF& poly) {
  const double x = poly.boundingRect().left();
  const double y = poly.boundingRect().top();
  const double shift_value_x = -(x - std::round(x));
  const double shift_value_y = -(y - std::round(y));

  return poly.translated(shift_value_x, shift_value_y);
}
}  // namespace page_layout