// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include <UnitsProvider.h>

#include <utility>
#include "Dpm.h"
#include "Filter.h"
#include "FilterUiInterface.h"
#include "ImageView.h"
#include "OptionsWidget.h"
#include "Settings.h"
#include "Task.h"
#include "TaskStatus.h"
#include "filters/page_split/Task.h"

namespace fix_orientation {
using imageproc::BinaryThreshold;

class Task::UiUpdater : public FilterResult {
 public:
  UiUpdater(intrusive_ptr<Filter> filter,
            const QImage& image,
            const ImageId& image_id,
            const ImageTransformation& xform,
            bool batch_processing);

  void updateUI(FilterUiInterface* ui) override;

  intrusive_ptr<AbstractFilter> filter() override { return m_filter; }

 private:
  intrusive_ptr<Filter> m_filter;
  QImage m_image;
  QImage m_downscaledImage;
  ImageId m_imageId;
  ImageTransformation m_xform;
  bool m_batchProcessing;
};


Task::Task(const PageId& page_id,
           intrusive_ptr<Filter> filter,
           intrusive_ptr<Settings> settings,
           intrusive_ptr<ImageSettings> image_settings,
           intrusive_ptr<page_split::Task> next_task,
           const bool batch_processing)
    : m_filter(std::move(filter)),
      m_nextTask(std::move(next_task)),
      m_settings(std::move(settings)),
      m_imageSettings(std::move(image_settings)),
      m_pageId(page_id),
      m_imageId(m_pageId.imageId()),
      m_batchProcessing(batch_processing) {}

Task::~Task() = default;

FilterResultPtr Task::process(const TaskStatus& status, FilterData data) {
  // This function is executed from the worker thread.

  status.throwIfCancelled();

  updateFilterData(data);

  ImageTransformation xform(data.xform());
  xform.setPreRotation(m_settings->getRotationFor(m_imageId));

  if (m_nextTask) {
    return m_nextTask->process(status, FilterData(data, xform));
  } else {
    return make_intrusive<UiUpdater>(m_filter, data.origImage(), m_imageId, xform, m_batchProcessing);
  }
}

void Task::updateFilterData(FilterData& data) {
  if (const std::unique_ptr<ImageSettings::PageParams> params = m_imageSettings->getPageParams(m_pageId)) {
    data.updateImageParams(*params);
  } else {
    ImageSettings::PageParams new_params(BinaryThreshold::otsuThreshold(data.grayImage()), true);

    m_imageSettings->setPageParams(m_pageId, new_params);
    data.updateImageParams(new_params);
  }
}

/*============================ Task::UiUpdater ========================*/

Task::UiUpdater::UiUpdater(intrusive_ptr<Filter> filter,
                           const QImage& image,
                           const ImageId& image_id,
                           const ImageTransformation& xform,
                           const bool batch_processing)
    : m_filter(std::move(filter)),
      m_image(image),
      m_downscaledImage(ImageView::createDownscaledImage(image)),
      m_imageId(image_id),
      m_xform(xform),
      m_batchProcessing(batch_processing) {}

void Task::UiUpdater::updateUI(FilterUiInterface* ui) {
  // This function is executed from the GUI thread.
  OptionsWidget* const opt_widget = m_filter->optionsWidget();
  opt_widget->postUpdateUI(m_xform.preRotation());
  ui->setOptionsWidget(opt_widget, ui->KEEP_OWNERSHIP);

  ui->invalidateThumbnail(PageId(m_imageId));

  if (m_batchProcessing) {
    return;
  }

  auto* view = new ImageView(m_image, m_downscaledImage, m_xform);
  ui->setImageWidget(view, ui->TRANSFER_OWNERSHIP);
  QObject::connect(opt_widget, SIGNAL(rotated(OrthogonalRotation)), view, SLOT(setPreRotation(OrthogonalRotation)));
}
}  // namespace fix_orientation