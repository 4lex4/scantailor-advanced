// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "CacheDrivenTask.h"

#include <core/ApplicationSettings.h>

#include <utility>

#include "IncompleteThumbnail.h"
#include "PageInfo.h"
#include "Params.h"
#include "Settings.h"
#include "Thumbnail.h"
#include "Utils.h"
#include "core/AbstractFilterDataCollector.h"
#include "core/ThumbnailCollector.h"
#include "filters/output/CacheDrivenTask.h"

namespace page_layout {
CacheDrivenTask::CacheDrivenTask(std::shared_ptr<output::CacheDrivenTask> nextTask, std::shared_ptr<Settings> settings)
    : m_nextTask(std::move(nextTask)), m_settings(std::move(settings)) {}

CacheDrivenTask::~CacheDrivenTask() = default;

void CacheDrivenTask::process(const PageInfo& pageInfo,
                              AbstractFilterDataCollector* collector,
                              const ImageTransformation& xform,
                              const QRectF& pageRect,
                              const QRectF& contentRect) {
  const std::unique_ptr<Params> params(m_settings->getPageParams(pageInfo.id()));
  if (!params || (params->contentSizeMM().isEmpty() && !contentRect.isEmpty())) {
    if (auto* thumbCol = dynamic_cast<ThumbnailCollector*>(collector)) {
      thumbCol->processThumbnail(std::unique_ptr<QGraphicsItem>(new IncompleteThumbnail(
          thumbCol->thumbnailCache(), thumbCol->maxLogicalThumbSize(), pageInfo.imageId(), xform)));
    }
    return;
  }

  Params newParams(
      m_settings->updateContentSizeAndGetParams(pageInfo.id(), pageRect, contentRect, params->contentSizeMM()));

  const QRectF adaptedContentRect(Utils::adaptContentRect(xform, contentRect));
  const QPolygonF contentRectPhys(xform.transformBack().map(adaptedContentRect));
  const QPolygonF pageRectPhys(
      Utils::calcPageRectPhys(xform, contentRectPhys, newParams, m_settings->getAggregateHardSizeMM()));

  ImageTransformation newXform(xform);
  newXform.setPostCropArea(Utils::shiftToRoundedOrigin(newXform.transform().map(pageRectPhys)));

  if (m_nextTask) {
    m_nextTask->process(pageInfo, collector, newXform, contentRectPhys);
    return;
  }

  ApplicationSettings& settings = ApplicationSettings::getInstance();
  const double deviationCoef = settings.getMarginsDeviationCoef();
  const double deviationThreshold = settings.getMarginsDeviationThreshold();

  if (auto* thumbCol = dynamic_cast<ThumbnailCollector*>(collector)) {
    thumbCol->processThumbnail(std::unique_ptr<QGraphicsItem>(new Thumbnail(
        thumbCol->thumbnailCache(), thumbCol->maxLogicalThumbSize(), pageInfo.imageId(), newParams, newXform,
        contentRectPhys, m_settings->deviationProvider().isDeviant(pageInfo.id(), deviationCoef, deviationThreshold))));
  }
}
}  // namespace page_layout