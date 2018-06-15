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
#include "Settings.h"
#include "Thumbnail.h"
#include "filter_dc/AbstractFilterDataCollector.h"
#include "filter_dc/ThumbnailCollector.h"
#include "filters/select_content/CacheDrivenTask.h"

namespace deskew {
CacheDrivenTask::CacheDrivenTask(intrusive_ptr<Settings> settings,
                                 intrusive_ptr<select_content::CacheDrivenTask> next_task)
    : m_ptrNextTask(std::move(next_task)), m_ptrSettings(std::move(settings)) {}

CacheDrivenTask::~CacheDrivenTask() = default;

void CacheDrivenTask::process(const PageInfo& page_info,
                              AbstractFilterDataCollector* collector,
                              const ImageTransformation& xform) {
  const Dependencies deps(xform.preCropArea(), xform.preRotation());
  std::unique_ptr<Params> params(m_ptrSettings->getPageParams(page_info.id()));
  if (!params || (!deps.matches(params->dependencies()) && (params->mode() == MODE_AUTO))) {
    if (auto* thumb_col = dynamic_cast<ThumbnailCollector*>(collector)) {
      thumb_col->processThumbnail(std::unique_ptr<QGraphicsItem>(new IncompleteThumbnail(
          thumb_col->thumbnailCache(), thumb_col->maxLogicalThumbSize(), page_info.imageId(), xform)));
    }

    return;
  }

  ImageTransformation new_xform(xform);
  new_xform.setPostRotation(params->deskewAngle());

  if (m_ptrNextTask) {
    m_ptrNextTask->process(page_info, collector, new_xform);

    return;
  }

  QSettings settings;
  const double deviationCoef = settings.value("settings/deskewDeviationCoef", 1.5).toDouble();
  const double deviationThreshold = settings.value("settings/deskewDeviationThreshold", 1.0).toDouble();

  if (auto* thumb_col = dynamic_cast<ThumbnailCollector*>(collector)) {
    thumb_col->processThumbnail(std::unique_ptr<QGraphicsItem>(new Thumbnail(
        thumb_col->thumbnailCache(), thumb_col->maxLogicalThumbSize(), page_info.imageId(), new_xform,
        m_ptrSettings->deviationProvider().isDeviant(page_info.id(), deviationCoef, deviationThreshold))));
  }
}  // CacheDrivenTask::process
}  // namespace deskew