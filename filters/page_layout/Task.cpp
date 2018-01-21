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

#include <UnitsProvider.h>
#include <DefaultParams.h>
#include <DefaultParamsProvider.h>
#include "Task.h"
#include "Filter.h"
#include "OptionsWidget.h"
#include "Settings.h"
#include "Params.h"
#include "Utils.h"
#include "FilterUiInterface.h"
#include "TaskStatus.h"
#include "FilterData.h"
#include "ImageView.h"
#include "filters/output/Task.h"
#include "Dpm.h"

#include "CommandLine.h"

namespace page_layout {
    class Task::UiUpdater : public FilterResult {
    public:
        UiUpdater(intrusive_ptr<Filter> const& filter,
                  intrusive_ptr<Settings> const& settings,
                  PageId const& page_id,
                  QImage const& image,
                  ImageTransformation const& xform,
                  QRectF const& adapted_content_rect,
                  bool agg_size_changed,
                  bool batch);

        virtual void updateUI(FilterUiInterface* ui);

        virtual intrusive_ptr<AbstractFilter> filter() {
            return m_ptrFilter;
        }

    private:
        intrusive_ptr<Filter> m_ptrFilter;
        intrusive_ptr<Settings> m_ptrSettings;
        PageId m_pageId;
        QImage m_image;
        QImage m_downscaledImage;
        ImageTransformation m_xform;
        QRectF m_adaptedContentRect;
        bool m_aggSizeChanged;
        bool m_batchProcessing;
    };


    Task::Task(intrusive_ptr<Filter> const& filter,
               intrusive_ptr<output::Task> const& next_task,
               intrusive_ptr<Settings> const& settings,
               PageId const& page_id,
               bool batch,
               bool debug)
            : m_ptrFilter(filter),
              m_ptrNextTask(next_task),
              m_ptrSettings(settings),
              m_pageId(page_id),
              m_batchProcessing(batch) {
    }

    Task::~Task() {
    }

    FilterResultPtr Task::process(TaskStatus const& status,
                                  FilterData const& data,
                                  QRectF const& page_rect,
                                  QRectF const& content_rect) {
        status.throwIfCancelled();

        loadDefaultSettings(Dpi(Dpm(data.origImage())));

        QSizeF const content_size_mm(
                Utils::calcRectSizeMM(data.xform(), content_rect)
        );

        if (m_ptrSettings->isPageAutoMarginsEnabled(m_pageId)) {
            Margins const& margins_mm = Utils::calcMarginsMM(data.xform(), page_rect, content_rect);
            m_ptrSettings->setHardMarginsMM(
                    m_pageId, margins_mm
            );
        }

        QSizeF agg_hard_size_before;
        QSizeF agg_hard_size_after;
        Params const params(
                m_ptrSettings->updateContentSizeAndGetParams(
                        m_pageId, page_rect, content_rect, content_size_mm,
                        &agg_hard_size_before, &agg_hard_size_after,
                        (m_ptrNextTask != nullptr)
                )
        );

        QRectF const adapted_content_rect(
                Utils::adaptContentRect(data.xform(), content_rect)
        );

        if (m_ptrNextTask) {
            QPolygonF const content_rect_phys(
                    data.xform().transformBack().map(adapted_content_rect)
            );
            QPolygonF const page_rect_phys(
                    Utils::calcPageRectPhys(
                            data.xform(), content_rect_phys,
                            params, agg_hard_size_after, m_ptrSettings->getContentRect()
                    )
            );

            ImageTransformation new_xform(data.xform());
            new_xform.setPostCropArea(new_xform.transform().map(page_rect_phys));

            return m_ptrNextTask->process(
                    status, FilterData(data, new_xform), content_rect_phys
            );
        } else {
            return FilterResultPtr(
                    new UiUpdater(
                            m_ptrFilter, m_ptrSettings, m_pageId,
                            data.origImage(), data.xform(), adapted_content_rect,
                            agg_hard_size_before != agg_hard_size_after,
                            m_batchProcessing
                    )
            );
        }
    }   // Task::process

    void Task::loadDefaultSettings(const Dpi& dpi) {
        if (m_ptrSettings->isParamsNull(m_pageId)
            || (m_ptrSettings->getHardMarginsMM(m_pageId).top() != -0.01)) {
            return;
        }

        const DefaultParams defaultParams = DefaultParamsProvider::getInstance()->getParams();
        const DefaultParams::PageLayoutParams& pageLayoutParams = defaultParams.getPageLayoutParams();

        UnitsConverter unitsConverter(dpi);

        const Margins& margins = pageLayoutParams.getHardMargins();
        double leftMargin = margins.left();
        double topMargin = margins.top();
        double rightMargin = margins.right();
        double bottomMargin = margins.bottom();
        unitsConverter.convert(leftMargin, topMargin, defaultParams.getUnits(), MILLIMETRES);
        unitsConverter.convert(rightMargin, bottomMargin, defaultParams.getUnits(), MILLIMETRES);

        m_ptrSettings->setHardMarginsMM(m_pageId, Margins(leftMargin, topMargin, rightMargin, bottomMargin));
    }

/*============================ Task::UiUpdater ==========================*/

    Task::UiUpdater::UiUpdater(intrusive_ptr<Filter> const& filter,
                               intrusive_ptr<Settings> const& settings,
                               PageId const& page_id,
                               QImage const& image,
                               ImageTransformation const& xform,
                               QRectF const& adapted_content_rect,
                               bool const agg_size_changed,
                               bool const batch)
            : m_ptrFilter(filter),
              m_ptrSettings(settings),
              m_pageId(page_id),
              m_image(image),
              m_downscaledImage(ImageView::createDownscaledImage(image)),
              m_xform(xform),
              m_adaptedContentRect(adapted_content_rect),
              m_aggSizeChanged(agg_size_changed),
              m_batchProcessing(batch) {
    }

    void Task::UiUpdater::updateUI(FilterUiInterface* ui) {
        // This function is executed from the GUI thread.
        UnitsProvider::getInstance()->setDpi(Dpi(Dpm(m_image)));

        OptionsWidget* const opt_widget = m_ptrFilter->optionsWidget();
        opt_widget->postUpdateUI();
        ui->setOptionsWidget(opt_widget, ui->KEEP_OWNERSHIP);

        if (m_aggSizeChanged) {
            ui->invalidateAllThumbnails();
        } else {
            ui->invalidateThumbnail(m_pageId);
        }

        if (m_batchProcessing) {
            return;
        }

        ImageView* view = new ImageView(
                m_ptrSettings, m_pageId, m_image, m_downscaledImage,
                m_xform, m_adaptedContentRect, *opt_widget
        );
        ui->setImageWidget(view, ui->TRANSFER_OWNERSHIP);

        QObject::connect(
                view, SIGNAL(invalidateThumbnail(PageId const &)),
                opt_widget, SIGNAL(invalidateThumbnail(PageId const &))
        );
        QObject::connect(
                view, SIGNAL(invalidateAllThumbnails()),
                opt_widget, SIGNAL(invalidateAllThumbnails())
        );
        QObject::connect(
                view, SIGNAL(marginsSetLocally(Margins const &)),
                opt_widget, SLOT(marginsSetExternally(Margins const &))
        );
        QObject::connect(
                opt_widget, SIGNAL(marginsSetLocally(Margins const &)),
                view, SLOT(marginsSetExternally(Margins const &))
        );
        QObject::connect(
                opt_widget, SIGNAL(topBottomLinkToggled(bool)),
                view, SLOT(topBottomLinkToggled(bool))
        );
        QObject::connect(
                opt_widget, SIGNAL(leftRightLinkToggled(bool)),
                view, SLOT(leftRightLinkToggled(bool))
        );
        QObject::connect(
                opt_widget, SIGNAL(alignmentChanged(Alignment const &)),
                view, SLOT(alignmentChanged(Alignment const &))
        );
        QObject::connect(
                opt_widget, SIGNAL(aggregateHardSizeChanged()),
                view, SLOT(aggregateHardSizeChanged())
        );
    }  // Task::UiUpdater::updateUI
}  // namespace page_layout