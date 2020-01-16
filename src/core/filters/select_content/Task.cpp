// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "Task.h"

#include <UnitsProvider.h>

#include <iostream>
#include <utility>

#include "ContentBoxFinder.h"
#include "DebugImagesImpl.h"
#include "Dpm.h"
#include "Filter.h"
#include "FilterData.h"
#include "FilterUiInterface.h"
#include "ImageView.h"
#include "OptionsWidget.h"
#include "PageFinder.h"
#include "TaskStatus.h"
#include "filters/page_layout/Task.h"

using namespace imageproc;

namespace select_content {
class Task::UiUpdater : public FilterResult {
 public:
  UiUpdater(intrusive_ptr<Filter> filter,
            const PageId& pageId,
            std::unique_ptr<DebugImages> dbg,
            const QImage& image,
            const ImageTransformation& xform,
            const GrayImage& grayImage,
            const OptionsWidget::UiData& uiData,
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
           intrusive_ptr<page_layout::Task> nextTask,
           intrusive_ptr<Settings> settings,
           const PageId& pageId,
           const bool batch,
           const bool debug)
    : m_filter(std::move(filter)),
      m_nextTask(std::move(nextTask)),
      m_settings(std::move(settings)),
      m_pageId(pageId),
      m_batchProcessing(batch) {
  if (debug) {
    m_dbg = std::make_unique<DebugImagesImpl>();
  }
}

Task::~Task() = default;

FilterResultPtr Task::process(const TaskStatus& status, const FilterData& data) {
  status.throwIfCancelled();

  std::unique_ptr<Params> params(m_settings->getPageParams(m_pageId));
  const Dependencies deps = (params) ? Dependencies(data.xform().resultingPreCropArea(), params->contentDetectionMode(),
                                                    params->pageDetectionMode(), params->isFineTuningEnabled())
                                     : Dependencies(data.xform().resultingPreCropArea());

  Params newParams(deps);
  if (params) {
    newParams = *params;
    newParams.setDependencies(deps);
  }

  const PhysSizeCalc physSizeCalc(data.xform());

  bool needUpdateContentBox = false;
  bool needUpdatePageBox = false;

  if (!params || !deps.compatibleWith(params->dependencies(), &needUpdateContentBox, &needUpdatePageBox)) {
    QRectF pageRect(newParams.pageRect());
    QRectF contentRect(newParams.contentRect());

    if (needUpdatePageBox) {
      if (newParams.pageDetectionMode() == MODE_AUTO) {
        pageRect
            = PageFinder::findPageBox(status, data, newParams.isFineTuningEnabled(), m_settings->pageDetectionBox(),
                                      m_settings->pageDetectionTolerance(), m_dbg.get());
      } else if (newParams.pageDetectionMode() == MODE_DISABLED) {
        pageRect = data.xform().resultingRect();
      }

      if (!data.xform().resultingRect().intersected(pageRect).isValid()) {
        pageRect = data.xform().resultingRect();
      }

      // Force update the content box if it doesn't fit into the page box updated.
      if (contentRect.isValid() && (contentRect.intersected(pageRect) != contentRect)) {
        needUpdateContentBox = true;
      }

      newParams.setPageRect(pageRect);
    }

    if (needUpdateContentBox) {
      if (newParams.contentDetectionMode() == MODE_AUTO) {
        contentRect = ContentBoxFinder::findContentBox(status, data, pageRect, m_dbg.get());
      } else if (newParams.contentDetectionMode() == MODE_DISABLED) {
        contentRect = pageRect;
      }

      if (contentRect.isValid()) {
        contentRect &= pageRect;
      }

      newParams.setContentRect(contentRect);
      newParams.setContentSizeMM(physSizeCalc.sizeMM(contentRect));
    }
  }

  OptionsWidget::UiData uiData;
  uiData.setSizeCalc(physSizeCalc);
  uiData.setContentRect(newParams.contentRect());
  uiData.setPageRect(newParams.pageRect());
  uiData.setDependencies(deps);
  uiData.setContentDetectionMode(newParams.contentDetectionMode());
  uiData.setPageDetectionMode(newParams.pageDetectionMode());
  uiData.setFineTuneCornersEnabled(newParams.isFineTuningEnabled());

  m_settings->setPageParams(m_pageId, newParams);

  status.throwIfCancelled();

  if (m_nextTask) {
    return m_nextTask->process(status, FilterData(data, data.xform()), uiData.pageRect(), uiData.contentRect());
  } else {
    return make_intrusive<UiUpdater>(m_filter, m_pageId, std::move(m_dbg), data.origImage(), data.xform(),
                                     data.grayImageBlackOnWhite(), uiData, m_batchProcessing);
  }
}  // Task::process

/*============================ Task::UiUpdater ==========================*/

Task::UiUpdater::UiUpdater(intrusive_ptr<Filter> filter,
                           const PageId& pageId,
                           std::unique_ptr<DebugImages> dbg,
                           const QImage& image,
                           const ImageTransformation& xform,
                           const GrayImage& grayImage,
                           const OptionsWidget::UiData& uiData,
                           const bool batch)
    : m_filter(std::move(filter)),
      m_pageId(pageId),
      m_dbg(std::move(dbg)),
      m_image(image),
      m_downscaledImage(ImageView::createDownscaledImage(image)),
      m_grayImage(grayImage),
      m_xform(xform),
      m_uiData(uiData),
      m_batchProcessing(batch) {}

void Task::UiUpdater::updateUI(FilterUiInterface* ui) {
  // This function is executed from the GUI thread.
  ui->invalidateThumbnail(m_pageId);

  if (m_batchProcessing) {
    return;
  }

  OptionsWidget* const optWidget = m_filter->optionsWidget();
  optWidget->postUpdateUI(m_uiData);
  ui->setOptionsWidget(optWidget, ui->KEEP_OWNERSHIP);

  auto* view = new ImageView(m_image, m_downscaledImage, m_grayImage, m_xform, m_uiData.contentRect(),
                             m_uiData.pageRect(), m_uiData.pageDetectionMode() != MODE_DISABLED);
  ui->setImageWidget(view, ui->TRANSFER_OWNERSHIP, m_dbg.get());

  QObject::connect(view, SIGNAL(manualContentRectSet(const QRectF&)), optWidget,
                   SLOT(manualContentRectSet(const QRectF&)));
  QObject::connect(view, SIGNAL(manualPageRectSet(const QRectF&)), optWidget, SLOT(manualPageRectSet(const QRectF&)));
  QObject::connect(view, SIGNAL(pageRectSizeChanged(const QSizeF&)), optWidget,
                   SLOT(updatePageRectSize(const QSizeF&)));
  QObject::connect(optWidget, SIGNAL(pageRectChangedLocally(const QRectF&)), view,
                   SLOT(pageRectSetExternally(const QRectF&)));
  QObject::connect(optWidget, SIGNAL(pageRectStateChanged(bool)), view, SLOT(setPageRectEnabled(bool)));
}
}  // namespace select_content