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

#include <utility>
#include "DebugImages.h"
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

  intrusive_ptr<AbstractFilter> filter() override { return m_ptrFilter; }

 private:
  intrusive_ptr<Filter> m_ptrFilter;
  intrusive_ptr<ProjectPages> m_ptrPages;
  std::unique_ptr<DebugImages> m_ptrDbg;
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
    : m_ptrFilter(std::move(filter)),
      m_ptrSettings(std::move(settings)),
      m_ptrPages(std::move(pages)),
      m_ptrNextTask(std::move(next_task)),
      m_pageInfo(page_info),
      m_batchProcessing(batch_processing) {
  if (debug) {
    m_ptrDbg = std::make_unique<DebugImages>();
  }
}

Task::~Task() = default;

FilterResultPtr Task::process(const TaskStatus& status, const FilterData& data) {
  status.throwIfCancelled();

  Settings::Record record(m_ptrSettings->getPageRecord(m_pageInfo.imageId()));

  const OrthogonalRotation pre_rotation(data.xform().preRotation());
  const Dependencies deps(data.origImage().size(), pre_rotation, record.combinedLayoutType());

  OptionsWidget::UiData ui_data;
  ui_data.setDependencies(deps);

  for (;;) {
    const Params* const params = record.params();

    PageLayout new_layout;
    std::unique_ptr<Params> new_params;

    if (!params || !deps.compatibleWith(*params)) {
      if (!params || ((record.layoutType() == nullptr) || (*record.layoutType() == AUTO_LAYOUT_TYPE))) {
        new_layout = PageLayoutEstimator::estimatePageLayout(record.combinedLayoutType(), data.grayImage(),
                                                             data.xform(), data.bwThreshold(), m_ptrDbg.get());

        status.throwIfCancelled();

        new_params = std::make_unique<Params>(new_layout, deps, MODE_AUTO);

        Dependencies newDeps = deps;
        newDeps.setLayoutType(new_layout.toLayoutType());
        new_params->setDependencies(newDeps);
      } else {
        new_params = std::make_unique<Params>(*params);

        std::unique_ptr<PageLayout> newPageLayout
            = PageLayoutAdapter::adaptPageLayout(params->pageLayout(), data.xform().resultingRect());

        new_params->setPageLayout(*newPageLayout);

        Dependencies newDeps = deps;
        newDeps.setLayoutType(newPageLayout->toLayoutType());
        new_params->setDependencies(newDeps);
      }
    } else {
      PageLayout correctedPageLayout = params->pageLayout();
      PageLayoutAdapter::correctPageLayoutType(&correctedPageLayout);
      if (correctedPageLayout.type() != params->pageLayout().type()) {
        new_params = std::make_unique<Params>(*params);
        new_params->setPageLayout(correctedPageLayout);

        Dependencies newDeps = deps;
        newDeps.setLayoutType(correctedPageLayout.toLayoutType());
        new_params->setDependencies(newDeps);
      } else {
        break;
      }
    }

    Settings::UpdateAction update;
    update.setLayoutType(new_params->pageLayout().toLayoutType());
    update.setParams(*new_params);

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
    record = m_ptrSettings->conditionalUpdate(m_pageInfo.imageId(), update, &conflict);
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

  const PageLayout& layout = record.params()->pageLayout();
  ui_data.setLayoutTypeAutoDetected(record.combinedLayoutType() == AUTO_LAYOUT_TYPE);
  ui_data.setPageLayout(layout);
  ui_data.setSplitLineMode(record.params()->splitLineMode());

  m_ptrPages->setLayoutTypeFor(m_pageInfo.imageId(), toPageLayoutType(layout));

  if (m_ptrNextTask != nullptr) {
    ImageTransformation new_xform(data.xform());
    new_xform.setPreCropArea(layout.pageOutline(m_pageInfo.id().subPage()).toPolygon());

    return m_ptrNextTask->process(status, FilterData(data, new_xform));
  }

  return make_intrusive<UiUpdater>(m_ptrFilter, m_ptrPages, std::move(m_ptrDbg), data.origImage(), m_pageInfo,
                                   data.xform(), ui_data, m_batchProcessing);
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
    : m_ptrFilter(std::move(filter)),
      m_ptrPages(std::move(pages)),
      m_ptrDbg(std::move(dbg_img)),
      m_image(image),
      m_downscaledImage(ImageView::createDownscaledImage(image)),
      m_pageInfo(page_info),
      m_xform(xform),
      m_uiData(ui_data),
      m_batchProcessing(batch_processing) {}

void Task::UiUpdater::updateUI(FilterUiInterface* ui) {
  // This function is executed from the GUI thread.
  OptionsWidget* const opt_widget = m_ptrFilter->optionsWidget();
  opt_widget->postUpdateUI(m_uiData);
  ui->setOptionsWidget(opt_widget, ui->KEEP_OWNERSHIP);

  ui->invalidateThumbnail(m_pageInfo.id());

  if (m_batchProcessing) {
    return;
  }

  auto view = new ImageView(m_image, m_downscaledImage, m_xform, m_uiData.pageLayout(), m_ptrPages,
                            m_pageInfo.imageId(), m_pageInfo.leftHalfRemoved(), m_pageInfo.rightHalfRemoved());
  ui->setImageWidget(view, ui->TRANSFER_OWNERSHIP, m_ptrDbg.get());

  QObject::connect(view, SIGNAL(invalidateThumbnail(const PageInfo&)), opt_widget,
                   SIGNAL(invalidateThumbnail(const PageInfo&)));
  QObject::connect(view, SIGNAL(pageLayoutSetLocally(const PageLayout&)), opt_widget,
                   SLOT(pageLayoutSetExternally(const PageLayout&)));
  QObject::connect(opt_widget, SIGNAL(pageLayoutSetLocally(const PageLayout&)), view,
                   SLOT(pageLayoutSetExternally(const PageLayout&)));
}  // Task::UiUpdater::updateUI
}  // namespace page_split