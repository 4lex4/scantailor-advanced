// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "CacheDrivenTask.h"

#include <core/ApplicationSettings.h>

#include <iostream>
#include <utility>

#include "IncompleteThumbnail.h"
#include "PageInfo.h"
#include "Settings.h"
#include "Thumbnail.h"
#include "core/AbstractFilterDataCollector.h"
#include "core/ContentBoxCollector.h"
#include "core/ThumbnailCollector.h"
#include "filters/page_layout/CacheDrivenTask.h"

namespace select_content {
CacheDrivenTask::CacheDrivenTask(intrusive_ptr<Settings> settings, intrusive_ptr<page_layout::CacheDrivenTask> nextTask)
    : m_settings(std::move(settings)), m_nextTask(std::move(nextTask)) {}

CacheDrivenTask::~CacheDrivenTask() = default;

void CacheDrivenTask::process(const PageInfo& pageInfo,
                              AbstractFilterDataCollector* collector,
                              const ImageTransformation& xform) {
  std::unique_ptr<Params> params(m_settings->getPageParams(pageInfo.id()));
  const Dependencies deps = (params) ? Dependencies(xform.resultingPreCropArea(), params->contentDetectionMode(),
                                                    params->pageDetectionMode(), params->isFineTuningEnabled())
                                     : Dependencies(xform.resultingPreCropArea());

  if (!params || !deps.compatibleWith(params->dependencies())) {
    if (auto* thumbCol = dynamic_cast<ThumbnailCollector*>(collector)) {
      thumbCol->processThumbnail(std::unique_ptr<QGraphicsItem>(new IncompleteThumbnail(
          thumbCol->thumbnailCache(), thumbCol->maxLogicalThumbSize(), pageInfo.imageId(), xform)));
    }
    return;
  }

  if (auto* col = dynamic_cast<ContentBoxCollector*>(collector)) {
    col->process(xform, params->contentRect());
  }

  if (m_nextTask) {
    m_nextTask->process(pageInfo, collector, xform, params->pageRect(), params->contentRect());
    return;
  }

  ApplicationSettings& settings = ApplicationSettings::getInstance();
  const double deviationCoef = settings.getSelectContentDeviationCoef();
  const double deviationThreshold = settings.getSelectContentDeviationThreshold();

  if (auto* thumbCol = dynamic_cast<ThumbnailCollector*>(collector)) {
    thumbCol->processThumbnail(std::unique_ptr<QGraphicsItem>(
        new Thumbnail(thumbCol->thumbnailCache(), thumbCol->maxLogicalThumbSize(), pageInfo.imageId(), xform,
                      params->contentRect(), params->pageRect(), params->pageDetectionMode() != MODE_DISABLED,
                      m_settings->deviationProvider().isDeviant(pageInfo.id(), deviationCoef, deviationThreshold))));
  }
}  // CacheDrivenTask::process
}  // namespace select_content