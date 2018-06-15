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
#include "IncompleteThumbnail.h"
#include "PageInfo.h"
#include "Settings.h"
#include "Thumbnail.h"
#include "filter_dc/AbstractFilterDataCollector.h"
#include "filter_dc/ContentBoxCollector.h"
#include "filter_dc/ThumbnailCollector.h"
#include "filters/page_layout/CacheDrivenTask.h"

#include <QtCore/QSettings>
#include <iostream>
#include <utility>

namespace select_content {
CacheDrivenTask::CacheDrivenTask(intrusive_ptr<Settings> settings,
                                 intrusive_ptr<page_layout::CacheDrivenTask> next_task)
    : m_ptrSettings(std::move(settings)), m_ptrNextTask(std::move(next_task)) {}

CacheDrivenTask::~CacheDrivenTask() = default;

void CacheDrivenTask::process(const PageInfo& page_info,
                              AbstractFilterDataCollector* collector,
                              const ImageTransformation& xform) {
  std::unique_ptr<Params> params(m_ptrSettings->getPageParams(page_info.id()));
  const Dependencies deps(xform.resultingPreCropArea());
  if (!params || !params->dependencies().matches(deps)) {
    if (auto* thumb_col = dynamic_cast<ThumbnailCollector*>(collector)) {
      thumb_col->processThumbnail(std::unique_ptr<QGraphicsItem>(new IncompleteThumbnail(
          thumb_col->thumbnailCache(), thumb_col->maxLogicalThumbSize(), page_info.imageId(), xform)));
    }

    return;
  }

  if (auto* col = dynamic_cast<ContentBoxCollector*>(collector)) {
    col->process(xform, params->contentRect());
  }

  if (m_ptrNextTask) {
    m_ptrNextTask->process(page_info, collector, xform, params->pageRect(), params->contentRect());

    return;
  }

  QSettings settings;
  const double deviationCoef = settings.value("settings/selectContentDeviationCoef", 0.35).toDouble();
  const double deviationThreshold = settings.value("settings/selectContentDeviationThreshold", 1.0).toDouble();

  if (auto* thumb_col = dynamic_cast<ThumbnailCollector*>(collector)) {
    thumb_col->processThumbnail(std::unique_ptr<QGraphicsItem>(new Thumbnail(
        thumb_col->thumbnailCache(), thumb_col->maxLogicalThumbSize(), page_info.imageId(), xform,
        params->contentRect(), params->pageRect(), params->isPageDetectionEnabled(),
        m_ptrSettings->deviationProvider().isDeviant(page_info.id(), deviationCoef, deviationThreshold))));
  }
}  // CacheDrivenTask::process
}  // namespace select_content