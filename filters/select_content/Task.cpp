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

#include "Task.h"
#include "Filter.h"
#include "FilterData.h"
#include "DebugImages.h"
#include "OptionsWidget.h"
#include "TaskStatus.h"
#include "ContentBoxFinder.h"
#include "PageFinder.h"
#include "FilterUiInterface.h"
#include "ImageView.h"
#include "filters/page_layout/Task.h"

#include <iostream>

namespace select_content {
    class Task::UiUpdater : public FilterResult {
    public:
        UiUpdater(IntrusivePtr<Filter> const& filter,
                  PageId const& page_id,
                  std::unique_ptr<DebugImages> dbg,
                  QImage const& image,
                  ImageTransformation const& xform,
                  OptionsWidget::UiData const& ui_data,
                  bool batch);

        virtual void updateUI(FilterUiInterface* ui);

        virtual IntrusivePtr<AbstractFilter> filter() {
            return m_ptrFilter;
        }

    private:
        IntrusivePtr<Filter> m_ptrFilter;
        PageId m_pageId;
        std::unique_ptr<DebugImages> m_ptrDbg;
        QImage m_image;
        QImage m_downscaledImage;
        ImageTransformation m_xform;
        OptionsWidget::UiData m_uiData;
        bool m_batchProcessing;
    };


    Task::Task(IntrusivePtr<Filter> const& filter,
               IntrusivePtr<page_layout::Task> const& next_task,
               IntrusivePtr<Settings> const& settings,
               PageId const& page_id,
               bool const batch,
               bool const debug)
            : m_ptrFilter(filter),
              m_ptrNextTask(next_task),
              m_ptrSettings(settings),
              m_pageId(page_id),
              m_batchProcessing(batch) {
        if (debug) {
            m_ptrDbg.reset(new DebugImages);
        }
    }

    Task::~Task() {
    }

    FilterResultPtr Task::process(TaskStatus const& status, FilterData const& data) {
        status.throwIfCancelled();

        Dependencies const deps(data.xform().resultingPreCropArea());

        OptionsWidget::UiData ui_data;
        ui_data.setSizeCalc(PhysSizeCalc(data.xform()));

        std::unique_ptr<Params> params(m_ptrSettings->getPageParams(m_pageId));
        Params new_params(deps);

        if (params.get()) {
            new_params = *params;
            new_params.setDependencies(deps);
        }

        if (!params.get() || !params->dependencies().matches(deps)) {
            QRectF page_rect(data.xform().resultingRect());
            QRectF content_rect(page_rect);

            // Backwards compatibilty: put the missing data where it belongs.
            if (new_params.isPageDetectionEnabled()) {
                page_rect = PageFinder::findPageBox(status, data, new_params.isFineTuningEnabled(),
                                                    m_ptrSettings->pageDetectionBox(),
                                                    m_ptrSettings->pageDetectionTolerance(), new_params.pageBorders(),
                                                    m_ptrDbg.get());
            }

            if (new_params.isContentDetectionEnabled() && (new_params.mode() == MODE_AUTO)) {
                content_rect = ContentBoxFinder::findContentBox(status, data, page_rect, m_ptrDbg.get());
            } else if (new_params.isContentDetectionEnabled() && (new_params.mode() == MODE_MANUAL)
                       && (content_rect = new_params.contentRect().intersected(page_rect)).isValid()) {
                // we don't want the content box to be out of the page bounds so use intersecting
            } else {
                content_rect = page_rect;
            }

            new_params.setPageRect(page_rect);
            new_params.setContentRect(content_rect);
        }

        ui_data.setContentRect(new_params.contentRect());
        ui_data.setPageRect(new_params.pageRect());
        ui_data.setDependencies(deps);
        ui_data.setMode(new_params.mode());
        ui_data.setContentDetection(new_params.isContentDetectionEnabled());
        ui_data.setPageDetection(new_params.isPageDetectionEnabled());
        ui_data.setFineTuneCorners(new_params.isFineTuningEnabled());
        ui_data.setPageBorders(new_params.pageBorders());

        new_params.setContentSizeMM(ui_data.contentSizeMM());

        new_params.computeDeviation(m_ptrSettings->avg());
        m_ptrSettings->setPageParams(m_pageId, new_params);

        status.throwIfCancelled();

        if (m_ptrNextTask) {
            return m_ptrNextTask->process(
                    status, FilterData(data, data.xform()),
                    ui_data.pageRect(), ui_data.contentRect()
            );
        } else {
            return FilterResultPtr(
                    new UiUpdater(
                            m_ptrFilter, m_pageId, std::move(m_ptrDbg), data.origImage(),
                            data.xform(), ui_data, m_batchProcessing
                    )
            );
        }
    }  // Task::process

/*============================ Task::UiUpdater ==========================*/

    Task::UiUpdater::UiUpdater(IntrusivePtr<Filter> const& filter,
                               PageId const& page_id,
                               std::unique_ptr<DebugImages> dbg,
                               QImage const& image,
                               ImageTransformation const& xform,
                               OptionsWidget::UiData const& ui_data,
                               bool const batch)
            : m_ptrFilter(filter),
              m_pageId(page_id),
              m_ptrDbg(std::move(dbg)),
              m_image(image),
              m_downscaledImage(ImageView::createDownscaledImage(image)),
              m_xform(xform),
              m_uiData(ui_data),
              m_batchProcessing(batch) {
    }

    void Task::UiUpdater::updateUI(FilterUiInterface* ui) {
        // This function is executed from the GUI thread.

        OptionsWidget* const opt_widget = m_ptrFilter->optionsWidget();
        opt_widget->postUpdateUI(m_uiData);
        ui->setOptionsWidget(opt_widget, ui->KEEP_OWNERSHIP);

        ui->invalidateThumbnail(m_pageId);

        if (m_batchProcessing) {
            return;
        }

        ImageView* view = new ImageView(
                m_image, m_downscaledImage,
                m_xform, m_uiData.contentRect(), m_uiData.pageRect()
        );
        ui->setImageWidget(view, ui->TRANSFER_OWNERSHIP, m_ptrDbg.get());

        QObject::connect(
                view, SIGNAL(manualContentRectSet(QRectF const &)),
                opt_widget, SLOT(manualContentRectSet(QRectF const &))
        );
    }
}  // namespace select_content