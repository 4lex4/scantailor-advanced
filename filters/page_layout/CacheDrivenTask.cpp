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

#include <utility>
#include "Settings.h"
#include "Params.h"
#include "Thumbnail.h"
#include "IncompleteThumbnail.h"
#include "PageInfo.h"
#include "Utils.h"
#include "filters/output/CacheDrivenTask.h"
#include "filter_dc/AbstractFilterDataCollector.h"
#include "filter_dc/ThumbnailCollector.h"

namespace page_layout {
    CacheDrivenTask::CacheDrivenTask(intrusive_ptr<output::CacheDrivenTask> next_task,
                                     intrusive_ptr<Settings> settings)
            : m_ptrNextTask(std::move(next_task)),
              m_ptrSettings(std::move(settings)) {
    }

    CacheDrivenTask::~CacheDrivenTask() = default;

    void CacheDrivenTask::process(PageInfo const& page_info,
                                  AbstractFilterDataCollector* collector,
                                  ImageTransformation const& xform,
                                  QRectF const& content_rect) {
        std::unique_ptr<Params> const params(
                m_ptrSettings->getPageParams(page_info.id())
        );
        if (!params || !params->contentSizeMM().isValid()) {
            if (auto* thumb_col = dynamic_cast<ThumbnailCollector*>(collector)) {
                thumb_col->processThumbnail(
                        std::unique_ptr<QGraphicsItem>(
                                new IncompleteThumbnail(
                                        thumb_col->thumbnailCache(),
                                        thumb_col->maxLogicalThumbSize(),
                                        page_info.imageId(), xform
                                )
                        )
                );
            }

            return;
        }

        QRectF const adapted_content_rect(
                Utils::adaptContentRect(xform, content_rect)
        );
        QPolygonF const content_rect_phys(
                xform.transformBack().map(adapted_content_rect)
        );
        QPolygonF const page_rect_phys(
                Utils::calcPageRectPhys(
                        xform, content_rect_phys, *params,
                        m_ptrSettings->getAggregateHardSizeMM(), m_ptrSettings->getContentRect()
                )
        );
        ImageTransformation new_xform(xform);
        new_xform.setPostCropArea(xform.transform().map(page_rect_phys));

        if (m_ptrNextTask) {
            m_ptrNextTask->process(page_info, collector, new_xform, content_rect_phys);

            return;
        }

        if (auto* thumb_col = dynamic_cast<ThumbnailCollector*>(collector)) {
            thumb_col->processThumbnail(
                    std::unique_ptr<QGraphicsItem>(
                            new Thumbnail(
                                    thumb_col->thumbnailCache(),
                                    thumb_col->maxLogicalThumbSize(),
                                    page_info.imageId(), *params,
                                    xform, content_rect_phys,
                                    xform.transform().map(page_rect_phys).boundingRect()
                            )
                    )
            );
        }
    }      // CacheDrivenTask::process
}  // namespace page_layout