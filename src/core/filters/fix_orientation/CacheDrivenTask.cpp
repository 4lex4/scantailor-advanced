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
CacheDrivenTask::CacheDrivenTask(intrusive_ptr<Settings> settings, intrusive_ptr<page_split::CacheDrivenTask> nextTask)
    : m_nextTask(std::move(nextTask)), m_settings(std::move(settings)) {}

CacheDrivenTask::~CacheDrivenTask() = default;

void CacheDrivenTask::process(const PageInfo& pageInfo, AbstractFilterDataCollector* collector) {
  const QRectF initialRect(QPointF(0.0, 0.0), pageInfo.metadata().size());
  ImageTransformation xform(initialRect, pageInfo.metadata().dpi());
  xform.setPreRotation(m_settings->getRotationFor(pageInfo.imageId()));

  if (auto* col = dynamic_cast<PageOrientationCollector*>(collector)) {
    col->process(xform.preRotation());
  }

  if (m_nextTask) {
    m_nextTask->process(pageInfo, collector, xform);

    return;
  }

  if (auto* thumbCol = dynamic_cast<ThumbnailCollector*>(collector)) {
    thumbCol->processThumbnail(std::unique_ptr<QGraphicsItem>(
        new ThumbnailBase(thumbCol->thumbnailCache(), thumbCol->maxLogicalThumbSize(), pageInfo.imageId(), xform)));
  }
}
}  // namespace fix_orientation