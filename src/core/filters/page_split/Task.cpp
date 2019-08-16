// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include <UnitsProvider.h>

#include <utility>
#include "DebugImagesImpl.h"
#include "Dpm.h"
#include "Filter.h"
#include "FilterData.h"
#include "FilterUiInterface.h"
#include "ImageView.h"
#include "OptionsWidget.h"
#include "PageLayoutAdapter.h"
#include "PageLayoutEstimator.h"
#include "ProjectPages.h"
#include "Settings.h"
#include "Task.h"
#include "TaskStatus.h"
#include "filters/deskew/Task.h"

namespace page_split {
using imageproc::BinaryThreshold;

class Task::UiUpdater : public FilterResult {
 public:
  UiUpdater(intrusive_ptr<Filter> filter,
            intrusive_ptr<ProjectPages> pages,
            std::unique_ptr<DebugImages> dbg_img,
            const QImage& image,
            const PageInfo& page_info,
            const ImageTransformation& xform,
            const OptionsWidget::UiData& ui_data,
            bool batch_processing);

  void updateUI(FilterUiInterface* ui) override;

  intrusive_ptr<AbstractFilter> filter() override { return m_filter; }

 private:
  intrusive_ptr<Filter> m_filter;
  intrusive_ptr<ProjectPages> m_pages;
  std::unique_ptr<DebugImages> m_dbg;
  QImage m_image;
  QImage m_downscaledImage;
  PageInfo m_pageInfo;
  ImageTransformation m_xform;
  OptionsWidget::UiData m_uiData;
  bool m_batchProcessing;
};


static ProjectPages::LayoutType toPageLayoutType(const PageLayout& layout) {
  switch (layout.type()) {
    case PageLayout::SINGLE_PAGE_UNCUT:
    case PageLayout::SINGLE_PAGE_CUT:
      return ProjectPages::ONE_PAGE_LAYOUT;
    case PageLayout::TWO_PAGES:
      return ProjectPages::TWO_PAGE_LAYOUT;
  }

  assert(!"Unreachable");

  return ProjectPages::ONE_PAGE_LAYOUT;
}

Task::Task(intrusive_ptr<Filter> filter,
           intrusive_ptr<Settings> settings,
           intrusive_ptr<ProjectPages> pages,
           intrusive_ptr<deskew::Task> next_task,
           const PageInfo& page_info,
           const bool batch_processing,
           const bool debug)
    : m_filter(std::move(filter)),
      m_settings(std::move(settings)),
      m_pages(std::move(pages)),
      m_nextTask(std::move(next_task)),
      m_pageInfo(page_info),
      m_batchProcessing(batch_processing) {
  if (debug) {
    m_dbg = std::make_unique<DebugImagesImpl>();
  }
}

Task::~Task() = default;

FilterResultPtr Task::process(const TaskStatus& status, const FilterData& data) {
  status.throwIfCancelled();

  Settings::Record record(m_settings->getPageRecord(m_pageInfo.imageId()));
  Dependencies deps(data.origImage().size(), data.xform().preRotation(), record.combinedLayoutType());

  while (true) {
    const Params* const params = record.params();

    LayoutType new_layout_type = record.combinedLayoutType();
    AutoManualMode split_line_mode = MODE_AUTO;
    PageLayout new_layout;

    if (!params || !deps.compatibleWith(*params)) {
      if (!params || (record.combinedLayoutType() == AUTO_LAYOUT_TYPE)) {
        new_layout = PageLayoutEstimator::estimatePageLayout(record.combinedLayoutType(), data.grayImage(),
                                                             data.xform(), data.bwThreshold(), m_dbg.get());

        status.throwIfCancelled();
      } else {
        new_layout = PageLayoutAdapter::adaptPageLayout(params->pageLayout(), data.xform().resultingRect());
        new_layout_type = new_layout.toLayoutType();
        split_line_mode = params->splitLineMode();
      }
    } else {
      PageLayout corrected_page_layout = params->pageLayout();
      PageLayoutAdapter::correctPageLayoutType(&corrected_page_layout);
      if (corrected_page_layout.type() == params->pageLayout().type()) {
        break;
      } else {
        new_layout = corrected_page_layout;
        new_layout_type = new_layout.toLayoutType();
        split_line_mode = params->splitLineMode();
      }
    }
    deps.setLayoutType(new_layout_type);
    const Params new_params(new_layout, deps, split_line_mode);

    Settings::UpdateAction update;
    update.setLayoutType(new_layout_type);
    update.setParams(new_params);

#ifndef NDEBUG
    {
      Settings::Record updated_record(record);
      updated_record.update(update);
      assert(!updated_record.hasLayoutTypeConflict());
      // This assert effectively verifies that PageLayoutEstimator::estimatePageLayout()
      // returned a layout with of a type consistent with the requested one.
      // If it didn't, it's a bug which will in fact cause a dead loop.
    }
#endif

    bool conflict = false;
    record = m_settings->conditionalUpdate(m_pageInfo.imageId(), update, &conflict);
    if (conflict && !record.params()) {
      // If there was a conflict, it means
      // the record was updated by another
      // thread somewhere between getPageRecord()
      // and conditionalUpdate().  If that
      // external update didn't leave page
      // parameters clear, we are just going
      // to use it's data, otherwise we need
      // to process this page again for the
      // new layout type.
      continue;
    }

    break;
  }

  OptionsWidget::UiData ui_data;
  ui_data.setDependencies(deps);
  const PageLayout& layout = record.params()->pageLayout();
  ui_data.setLayoutTypeAutoDetected(record.combinedLayoutType() == AUTO_LAYOUT_TYPE);
  ui_data.setPageLayout(layout);
  ui_data.setSplitLineMode(record.params()->splitLineMode());

  m_pages->setLayoutTypeFor(m_pageInfo.imageId(), toPageLayoutType(layout));

  if (m_nextTask != nullptr) {
    ImageTransformation new_xform(data.xform());
    new_xform.setPreCropArea(layout.pageOutline(m_pageInfo.id().subPage()).toPolygon());

    return m_nextTask->process(status, FilterData(data, new_xform));
  }

  return make_intrusive<UiUpdater>(m_filter, m_pages, std::move(m_dbg), data.origImage(), m_pageInfo, data.xform(),
                                   ui_data, m_batchProcessing);
}  // Task::process

/*============================ Task::UiUpdater =========================*/

Task::UiUpdater::UiUpdater(intrusive_ptr<Filter> filter,
                           intrusive_ptr<ProjectPages> pages,
                           std::unique_ptr<DebugImages> dbg_img,
                           const QImage& image,
                           const PageInfo& page_info,
                           const ImageTransformation& xform,
                           const OptionsWidget::UiData& ui_data,
                           const bool batch_processing)
    : m_filter(std::move(filter)),
      m_pages(std::move(pages)),
      m_dbg(std::move(dbg_img)),
      m_image(image),
      m_downscaledImage(ImageView::createDownscaledImage(image)),
      m_pageInfo(page_info),
      m_xform(xform),
      m_uiData(ui_data),
      m_batchProcessing(batch_processing) {}

void Task::UiUpdater::updateUI(FilterUiInterface* ui) {
  // This function is executed from the GUI thread.
  OptionsWidget* const opt_widget = m_filter->optionsWidget();
  opt_widget->postUpdateUI(m_uiData);
  ui->setOptionsWidget(opt_widget, ui->KEEP_OWNERSHIP);

  ui->invalidateThumbnail(m_pageInfo.id());

  if (m_batchProcessing) {
    return;
  }

  auto view = new ImageView(m_image, m_downscaledImage, m_xform, m_uiData.pageLayout(), m_pages, m_pageInfo.imageId(),
                            m_pageInfo.leftHalfRemoved(), m_pageInfo.rightHalfRemoved());
  ui->setImageWidget(view, ui->TRANSFER_OWNERSHIP, m_dbg.get());

  QObject::connect(view, SIGNAL(invalidateThumbnail(const PageInfo&)), opt_widget,
                   SIGNAL(invalidateThumbnail(const PageInfo&)));
  QObject::connect(view, SIGNAL(pageLayoutSetLocally(const PageLayout&)), opt_widget,
                   SLOT(pageLayoutSetExternally(const PageLayout&)));
  QObject::connect(opt_widget, SIGNAL(pageLayoutSetLocally(const PageLayout&)), view,
                   SLOT(pageLayoutSetExternally(const PageLayout&)));
}  // Task::UiUpdater::updateUI
}  // namespace page_split