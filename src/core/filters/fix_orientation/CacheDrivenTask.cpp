// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "CacheDrivenTask.h"

#include <utility>
#include "ImageTransformation.h"
#include "PageInfo.h"
#include "Settings.h"
#include "ThumbnailBase.h"
#include "core/AbstractFilterDataCollector.h"
#include "core/PageOrientationCollector.h"
#include "core/ThumbnailCollector.h"
#include "filters/page_split/CacheDrivenTask.h"

namespace fix_orientation {
CacheDrivenTask::CacheDrivenTask(intrusive_ptr<Settings> settings, intrusive_ptr<page_split::CacheDrivenTask> next_task)
    : m_nextTask(std::move(next_task)), m_settings(std::move(settings)) {}

CacheDrivenTask::~CacheDrivenTask() = default;

void CacheDrivenTask::process(const PageInfo& page_info, AbstractFilterDataCollector* collector) {
  const QRectF initial_rect(QPointF(0.0, 0.0), page_info.metadata().size());
  ImageTransformation xform(initial_rect, page_info.metadata().dpi());
  xform.setPreRotation(m_settings->getRotationFor(page_info.imageId()));

  if (auto* col = dynamic_cast<PageOrientationCollector*>(collector)) {
    col->process(xform.preRotation());
  }

  if (m_nextTask) {
    m_nextTask->process(page_info, collector, xform);

    return;
  }

  if (auto* thumb_col = dynamic_cast<ThumbnailCollector*>(collector)) {
    thumb_col->processThumbnail(std::unique_ptr<QGraphicsItem>(
        new ThumbnailBase(thumb_col->thumbnailCache(), thumb_col->maxLogicalThumbSize(), page_info.imageId(), xform)));
  }
}
}  // namespace fix_orientation