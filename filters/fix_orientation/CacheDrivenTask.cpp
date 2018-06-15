/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2007-2008  Joseph Artsimovich <joseph_a@mail.ru>

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

#include <utility>
#include "ImageTransformation.h"
#include "PageInfo.h"
#include "Settings.h"
#include "ThumbnailBase.h"
#include "filter_dc/AbstractFilterDataCollector.h"
#include "filter_dc/PageOrientationCollector.h"
#include "filter_dc/ThumbnailCollector.h"
#include "filters/page_split/CacheDrivenTask.h"

namespace fix_orientation {
CacheDrivenTask::CacheDrivenTask(intrusive_ptr<Settings> settings, intrusive_ptr<page_split::CacheDrivenTask> next_task)
    : m_ptrNextTask(std::move(next_task)), m_ptrSettings(std::move(settings)) {}

CacheDrivenTask::~CacheDrivenTask() = default;

void CacheDrivenTask::process(const PageInfo& page_info, AbstractFilterDataCollector* collector) {
  const QRectF initial_rect(QPointF(0.0, 0.0), page_info.metadata().size());
  ImageTransformation xform(initial_rect, page_info.metadata().dpi());
  xform.setPreRotation(m_ptrSettings->getRotationFor(page_info.imageId()));

  if (auto* col = dynamic_cast<PageOrientationCollector*>(collector)) {
    col->process(xform.preRotation());
  }

  if (m_ptrNextTask) {
    m_ptrNextTask->process(page_info, collector, xform);

    return;
  }

  if (auto* thumb_col = dynamic_cast<ThumbnailCollector*>(collector)) {
    thumb_col->processThumbnail(std::unique_ptr<QGraphicsItem>(
        new ThumbnailBase(thumb_col->thumbnailCache(), thumb_col->maxLogicalThumbSize(), page_info.imageId(), xform)));
  }
}
}  // namespace fix_orientation