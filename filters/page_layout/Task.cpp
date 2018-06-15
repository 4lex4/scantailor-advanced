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
#include <UnitsProvider.h>
#include <utility>
#include "Dpm.h"
#include "Filter.h"
#include "FilterData.h"
#include "FilterUiInterface.h"
#include "ImageView.h"
#include "OptionsWidget.h"
#include "Params.h"
#include "Settings.h"
#include "TaskStatus.h"
#include "Utils.h"
#include "filters/output/Task.h"

using namespace imageproc;

namespace page_layout {
class Task::UiUpdater : public FilterResult {
 public:
  UiUpdater(intrusive_ptr<Filter> filter,
            intrusive_ptr<Settings> settings,
            const PageId& page_id,
            const QImage& image,
            const ImageTransformation& xform,
            const GrayImage& gray_image,
            const QRectF& adapted_content_rect,
            bool agg_size_changed,
            bool batch);

  void updateUI(FilterUiInterface* ui) override;

  intrusive_ptr<AbstractFilter> filter() override { return m_ptrFilter; }

 private:
  intrusive_ptr<Filter> m_ptrFilter;
  intrusive_ptr<Settings> m_ptrSettings;
  PageId m_pageId;
  QImage m_image;
  QImage m_downscaledImage;
  GrayImage m_grayImage;
  ImageTransformation m_xform;
  QRectF m_adaptedContentRect;
  bool m_aggSizeChanged;
  bool m_batchProcessing;
};


Task::Task(intrusive_ptr<Filter> filter,
           intrusive_ptr<output::Task> next_task,
           intrusive_ptr<Settings> settings,
           const PageId& page_id,
           bool batch,
           bool debug)
    : m_ptrFilter(std::move(filter)),
      m_ptrNextTask(std::move(next_task)),
      m_ptrSettings(std::move(settings)),
      m_pageId(page_id),
      m_batchProcessing(batch) {}

Task::~Task() = default;

FilterResultPtr Task::process(const TaskStatus& status,
                              const FilterData& data,
                              const QRectF& page_rect,
                              const QRectF& content_rect) {
  status.throwIfCancelled();

  const QSizeF content_size_mm(Utils::calcRectSizeMM(data.xform(), content_rect));

  if (m_ptrSettings->isPageAutoMarginsEnabled(m_pageId)) {
    const Margins& margins_mm = Utils::calcMarginsMM(data.xform(), page_rect, content_rect);
    m_ptrSettings->setHardMarginsMM(m_pageId, margins_mm);
  }

  QSizeF agg_hard_size_before;
  QSizeF agg_hard_size_after;
  const Params params(m_ptrSettings->updateContentSizeAndGetParams(m_pageId, page_rect, content_rect, content_size_mm,
                                                                   &agg_hard_size_before, &agg_hard_size_after));

  const QRectF adapted_content_rect(Utils::adaptContentRect(data.xform(), content_rect));

  if (m_ptrNextTask) {
    const QPolygonF content_rect_phys(data.xform().transformBack().map(adapted_content_rect));
    const QPolygonF page_rect_phys(Utils::calcPageRectPhys(data.xform(), content_rect_phys, params, agg_hard_size_after,
                                                           m_ptrSettings->getAggregateContentRect()));

    ImageTransformation new_xform(data.xform());
    new_xform.setPostCropArea(shiftToRoundedOrigin(new_xform.transform().map(page_rect_phys)));

    return m_ptrNextTask->process(status, FilterData(data, new_xform), content_rect_phys);
  } else {
    return make_intrusive<UiUpdater>(m_ptrFilter, m_ptrSettings, m_pageId, data.origImage(), data.xform(),
                                     data.isBlackOnWhite() ? data.grayImage() : data.grayImage().inverted(),
                                     adapted_content_rect, agg_hard_size_before != agg_hard_size_after,
                                     m_batchProcessing);
  }
}

QPolygonF Task::shiftToRoundedOrigin(const QPolygonF& poly) {
  const double x = poly.boundingRect().left();
  const double y = poly.boundingRect().top();
  const double shift_value_x = -(x - std::round(x));
  const double shift_value_y = -(y - std::round(y));

  return poly.translated(shift_value_x, shift_value_y);
}

/*============================ Task::UiUpdater ==========================*/

Task::UiUpdater::UiUpdater(intrusive_ptr<Filter> filter,
                           intrusive_ptr<Settings> settings,
                           const PageId& page_id,
                           const QImage& image,
                           const ImageTransformation& xform,
                           const GrayImage& gray_image,
                           const QRectF& adapted_content_rect,
                           const bool agg_size_changed,
                           const bool batch)
    : m_ptrFilter(std::move(filter)),
      m_ptrSettings(std::move(settings)),
      m_pageId(page_id),
      m_image(image),
      m_downscaledImage(ImageView::createDownscaledImage(image)),
      m_xform(xform),
      m_grayImage(gray_image),
      m_adaptedContentRect(adapted_content_rect),
      m_aggSizeChanged(agg_size_changed),
      m_batchProcessing(batch) {}

void Task::UiUpdater::updateUI(FilterUiInterface* ui) {
  // This function is executed from the GUI thread.
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

  auto* view = new ImageView(m_ptrSettings, m_pageId, m_image, m_downscaledImage, m_grayImage, m_xform,
                             m_adaptedContentRect, *opt_widget);
  ui->setImageWidget(view, ui->TRANSFER_OWNERSHIP);

  QObject::connect(view, SIGNAL(invalidateThumbnail(const PageId&)), opt_widget,
                   SIGNAL(invalidateThumbnail(const PageId&)));
  QObject::connect(view, SIGNAL(invalidateAllThumbnails()), opt_widget, SIGNAL(invalidateAllThumbnails()));
  QObject::connect(view, SIGNAL(marginsSetLocally(const Margins&)), opt_widget,
                   SLOT(marginsSetExternally(const Margins&)));
  QObject::connect(opt_widget, SIGNAL(marginsSetLocally(const Margins&)), view,
                   SLOT(marginsSetExternally(const Margins&)));
  QObject::connect(opt_widget, SIGNAL(topBottomLinkToggled(bool)), view, SLOT(topBottomLinkToggled(bool)));
  QObject::connect(opt_widget, SIGNAL(leftRightLinkToggled(bool)), view, SLOT(leftRightLinkToggled(bool)));
  QObject::connect(opt_widget, SIGNAL(alignmentChanged(const Alignment&)), view,
                   SLOT(alignmentChanged(const Alignment&)));
  QObject::connect(opt_widget, SIGNAL(aggregateHardSizeChanged()), view, SLOT(aggregateHardSizeChanged()));
}  // Task::UiUpdater::updateUI
}  // namespace page_layout