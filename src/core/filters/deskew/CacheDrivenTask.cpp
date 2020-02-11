// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "CacheDrivenTask.h"

#include <core/ApplicationSettings.h>

#include <utility>

#include "AbstractFilterDataCollector.h"
#include "IncompleteThumbnail.h"
#include "PageInfo.h"
#include "Settings.h"
#include "Thumbnail.h"
#include "ThumbnailCollector.h"
#include "filters/select_content/CacheDrivenTask.h"

namespace deskew {
CacheDrivenTask::CacheDrivenTask(std::shared_ptr<Settings> settings,
                                 std::shared_ptr<select_content::CacheDrivenTask> nextTask)
    : m_nextTask(std::move(nextTask)), m_settings(std::move(settings)) {}

CacheDrivenTask::~CacheDrivenTask() = default;

void CacheDrivenTask::process(const PageInfo& pageInfo,
                              AbstractFilterDataCollector* collector,
                              const ImageTransformation& xform) {
  const Dependencies deps(xform.preCropArea(), xform.preRotation());
  std::unique_ptr<Params> params(m_settings->getPageParams(pageInfo.id()));
  if (!params || (!deps.matches(params->dependencies()) && (params->mode() == MODE_AUTO))) {
    if (auto* thumbCol = dynamic_cast<ThumbnailCollector*>(collector)) {
      thumbCol->processThumbnail(std::unique_ptr<QGraphicsItem>(new IncompleteThumbnail(
          thumbCol->thumbnailCache(), thumbCol->maxLogicalThumbSize(), pageInfo.imageId(), xform)));
    }
    return;
  }

  ImageTransformation newXform(xform);
  newXform.setPostRotation(params->deskewAngle());

  if (m_nextTask) {
    m_nextTask->process(pageInfo, collector, newXform);
    return;
  }

  ApplicationSettings& settings = ApplicationSettings::getInstance();
  const double deviationCoef = settings.getDeskewDeviationCoef();
  const double deviationThreshold = settings.getDeskewDeviationThreshold();

  if (auto* thumbCol = dynamic_cast<ThumbnailCollector*>(collector)) {
    thumbCol->processThumbnail(std::unique_ptr<QGraphicsItem>(
        new Thumbnail(thumbCol->thumbnailCache(), thumbCol->maxLogicalThumbSize(), pageInfo.imageId(), newXform,
                      m_settings->deviationProvider().isDeviant(pageInfo.id(), deviationCoef, deviationThreshold))));
  }
}  // CacheDrivenTask::process
}  // namespace deskew