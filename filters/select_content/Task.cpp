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
#include "ContentBoxFinder.h"
#include "DebugImages.h"
#include "Filter.h"
#include "FilterData.h"
#include "FilterUiInterface.h"
#include "ImageView.h"
#include "OptionsWidget.h"
#include "PageFinder.h"
#include "TaskStatus.h"
#include "filters/page_layout/Task.h"

#include <UnitsProvider.h>
#include <iostream>
#include <utility>
#include "Dpm.h"

using namespace imageproc;

namespace select_content {
class Task::UiUpdater : public FilterResult {
 public:
  UiUpdater(intrusive_ptr<Filter> filter,
            const PageId& page_id,
            std::unique_ptr<DebugImages> dbg,
            const QImage& image,
            const ImageTransformation& xform,
            const GrayImage& gray_image,
            const OptionsWidget::UiData& ui_data,
            bool batch);

  void updateUI(FilterUiInterface* ui) override;

  intrusive_ptr<AbstractFilter> filter() override { return m_filter; }

 private:
  intrusive_ptr<Filter> m_filter;
  PageId m_pageId;
  std::unique_ptr<DebugImages> m_dbg;
  QImage m_image;
  QImage m_downscaledImage;
  GrayImage m_grayImage;
  ImageTransformation m_xform;
  OptionsWidget::UiData m_uiData;
  bool m_batchProcessing;
};


Task::Task(intrusive_ptr<Filter> filter,
           intrusive_ptr<page_layout::Task> next_task,
           intrusive_ptr<Settings> settings,
           const PageId& page_id,
           const bool batch,
           const bool debug)
    : m_filter(std::move(filter)),
      m_nextTask(std::move(next_task)),
      m_settings(std::move(settings)),
      m_pageId(page_id),
      m_batchProcessing(batch) {
  if (debug) {
    m_dbg = std::make_unique<DebugImages>();
  }
}

Task::~Task() = default;

FilterResultPtr Task::process(const TaskStatus& status, const FilterData& data) {
  status.throwIfCancelled();

  std::unique_ptr<Params> params(m_settings->getPageParams(m_pageId));
  const Dependencies deps = (params) ? Dependencies(data.xform().resultingPreCropArea(), params->contentDetectionMode(),
                                                    params->pageDetectionMode(), params->isFineTuningEnabled())
                                     : Dependencies(data.xform().resultingPreCropArea());

  Params new_params(deps);
  if (params) {
    new_params = *params;
    new_params.setDependencies(deps);
  }

  const PhysSizeCalc phys_size_calc(data.xform());

  bool need_update_content_box = false;
  bool need_update_page_box = false;

  if (!params || !deps.compatibleWith(params->dependencies(), &need_update_content_box, &need_update_page_box)) {
    QRectF page_rect(new_params.pageRect());
    QRectF content_rect(new_params.contentRect());

    if (need_update_page_box) {
      if (new_params.pageDetectionMode() == MODE_AUTO) {
        page_rect
            = PageFinder::findPageBox(status, data, new_params.isFineTuningEnabled(), m_settings->pageDetectionBox(),
                                      m_settings->pageDetectionTolerance(), m_dbg.get());
      } else if (new_params.pageDetectionMode() == MODE_DISABLED) {
        page_rect = data.xform().resultingRect();
      }

      if (!data.xform().resultingRect().intersected(page_rect).isValid()) {
        page_rect = data.xform().resultingRect();
      }

      // Force update the content box if it doesn't fit into the page box updated.
      if (content_rect.isValid() && (content_rect.intersected(page_rect)) != content_rect) {
        need_update_content_box = true;
      }

      new_params.setPageRect(page_rect);
    }

    if (need_update_content_box) {
      if (new_params.contentDetectionMode() == MODE_AUTO) {
        content_rect = ContentBoxFinder::findContentBox(status, data, page_rect, m_dbg.get());
      } else if (new_params.contentDetectionMode() == MODE_DISABLED) {
        content_rect = page_rect;
      }

      if (content_rect.isValid()) {
        content_rect &= page_rect;
      }

      new_params.setContentRect(content_rect);
      new_params.setContentSizeMM(phys_size_calc.sizeMM(content_rect));
    }
  }

  OptionsWidget::UiData ui_data;
  ui_data.setSizeCalc(phys_size_calc);
  ui_data.setContentRect(new_params.contentRect());
  ui_data.setPageRect(new_params.pageRect());
  ui_data.setDependencies(deps);
  ui_data.setContentDetectionMode(new_params.contentDetectionMode());
  ui_data.setPageDetectionMode(new_params.pageDetectionMode());
  ui_data.setFineTuneCornersEnabled(new_params.isFineTuningEnabled());

  m_settings->setPageParams(m_pageId, new_params);

  status.throwIfCancelled();

  if (m_nextTask) {
    return m_nextTask->process(status, FilterData(data, data.xform()), ui_data.pageRect(), ui_data.contentRect());
  } else {
    return make_intrusive<UiUpdater>(m_filter, m_pageId, std::move(m_dbg), data.origImage(), data.xform(),
                                     data.isBlackOnWhite() ? data.grayImage() : data.grayImage().inverted(), ui_data,
                                     m_batchProcessing);
  }
}  // Task::process

/*============================ Task::UiUpdater ==========================*/

Task::UiUpdater::UiUpdater(intrusive_ptr<Filter> filter,
                           const PageId& page_id,
                           std::unique_ptr<DebugImages> dbg,
                           const QImage& image,
                           const ImageTransformation& xform,
                           const GrayImage& gray_image,
                           const OptionsWidget::UiData& ui_data,
                           const bool batch)
    : m_filter(std::move(filter)),
      m_pageId(page_id),
      m_dbg(std::move(dbg)),
      m_image(image),
      m_downscaledImage(ImageView::createDownscaledImage(image)),
      m_xform(xform),
      m_grayImage(gray_image),
      m_uiData(ui_data),
      m_batchProcessing(batch) {}

void Task::UiUpdater::updateUI(FilterUiInterface* ui) {
  // This function is executed from the GUI thread.
  OptionsWidget* const opt_widget = m_filter->optionsWidget();
  opt_widget->postUpdateUI(m_uiData);
  ui->setOptionsWidget(opt_widget, ui->KEEP_OWNERSHIP);

  ui->invalidateThumbnail(m_pageId);

  if (m_batchProcessing) {
    return;
  }

  auto* view = new ImageView(m_image, m_downscaledImage, m_grayImage, m_xform, m_uiData.contentRect(),
                             m_uiData.pageRect(), m_uiData.pageDetectionMode() != MODE_DISABLED);
  ui->setImageWidget(view, ui->TRANSFER_OWNERSHIP, m_dbg.get());

  QObject::connect(view, SIGNAL(manualContentRectSet(const QRectF&)), opt_widget,
                   SLOT(manualContentRectSet(const QRectF&)));
  QObject::connect(view, SIGNAL(manualPageRectSet(const QRectF&)), opt_widget, SLOT(manualPageRectSet(const QRectF&)));
  QObject::connect(view, SIGNAL(pageRectSizeChanged(const QSizeF&)), opt_widget,
                   SLOT(updatePageRectSize(const QSizeF&)));
  QObject::connect(opt_widget, SIGNAL(pageRectChangedLocally(const QRectF&)), view,
                   SLOT(pageRectSetExternally(const QRectF&)));
  QObject::connect(opt_widget, SIGNAL(pageRectStateChanged(bool)), view, SLOT(setPageRectEnabled(bool)));
}
}  // namespace select_content