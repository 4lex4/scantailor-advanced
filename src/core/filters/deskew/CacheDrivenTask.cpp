// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "CacheDrivenTask.h"

#include <utility>
#include <core/ApplicationSettings.h>
#include "IncompleteThumbnail.h"
#include "PageInfo.h"
#include "Settings.h"
#include "Thumbnail.h"
#include "AbstractFilterDataCollector.h"
#include "ThumbnailCollector.h"
#include "filters/select_content/CacheDrivenTask.h"

namespace deskew {
CacheDrivenTask::CacheDrivenTask(intrusive_ptr<Settings> settings,
                                 intrusive_ptr<select_content::CacheDrivenTask> next_task)
    : m_nextTask(std::move(next_task)), m_settings(std::move(settings)) {}

CacheDrivenTask::~CacheDrivenTask() = default;

void CacheDrivenTask::process(const PageInfo& page_info,
                              AbstractFilterDataCollector* collector,
                              const ImageTransformation& xform) {
  const Dependencies deps(xform.preCropArea(), xform.preRotation());
  std::unique_ptr<Params> params(m_settings->getPageParams(page_info.id()));
  if (!params || (!deps.matches(params->dependencies()) && (params->mode() == MODE_AUTO))) {
    if (auto* thumb_col = dynamic_cast<ThumbnailCollector*>(collector)) {
      thumb_col->processThumbnail(std::unique_ptr<QGraphicsItem>(new IncompleteThumbnail(
          thumb_col->thumbnailCache(), thumb_col->maxLogicalThumbSize(), page_info.imageId(), xform)));
    }

    return;
  }

  ImageTransformation new_xform(xform);
  new_xform.setPostRotation(params->deskewAngle());

  if (m_nextTask) {
    m_nextTask->process(page_info, collector, new_xform);

    return;
  }

  ApplicationSettings& settings = ApplicationSettings::getInstance();
  const double deviationCoef = settings.getDeskewDeviationCoef();
  const double deviationThreshold = settings.getDeskewDeviationThreshold();

  if (auto* thumb_col = dynamic_cast<ThumbnailCollector*>(collector)) {
    thumb_col->processThumbnail(std::unique_ptr<QGraphicsItem>(
        new Thumbnail(thumb_col->thumbnailCache(), thumb_col->maxLogicalThumbSize(), page_info.imageId(), new_xform,
                      m_settings->deviationProvider().isDeviant(page_info.id(), deviationCoef, deviationThreshold))));
  }
}  // CacheDrivenTask::process
}  // namespace deskew