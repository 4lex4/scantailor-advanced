// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "Task.h"

#include <UnitsProvider.h>

#include <utility>

#include "Dpm.h"
#include "Filter.h"
#include "FilterUiInterface.h"
#include "ImageView.h"
#include "OptionsWidget.h"
#include "Settings.h"
#include "TaskStatus.h"
#include "filters/page_split/Task.h"

namespace fix_orientation {
using imageproc::BinaryThreshold;

class Task::UiUpdater : public FilterResult {
 public:
  UiUpdater(std::shared_ptr<Filter> filter,
            const QImage& image,
            const ImageId& imageId,
            const ImageTransformation& xform,
            bool batchProcessing);

  void updateUI(FilterUiInterface* ui) override;

  std::shared_ptr<AbstractFilter> filter() override { return m_filter; }

 private:
  std::shared_ptr<Filter> m_filter;
  QImage m_image;
  QImage m_downscaledImage;
  ImageId m_imageId;
  ImageTransformation m_xform;
  bool m_batchProcessing;
};


Task::Task(const PageId& pageId,
           std::shared_ptr<Filter> filter,
           std::shared_ptr<Settings> settings,
           std::shared_ptr<ImageSettings> imageSettings,
           std::shared_ptr<page_split::Task> nextTask,
           const bool batchProcessing)
    : m_filter(std::move(filter)),
      m_nextTask(std::move(nextTask)),
      m_settings(std::move(settings)),
      m_imageSettings(std::move(imageSettings)),
      m_pageId(pageId),
      m_imageId(m_pageId.imageId()),
      m_batchProcessing(batchProcessing) {}

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
    return std::make_shared<UiUpdater>(m_filter, data.origImage(), m_imageId, xform, m_batchProcessing);
  }
}

void Task::updateFilterData(FilterData& data) {
  if (const std::unique_ptr<ImageSettings::PageParams> params = m_imageSettings->getPageParams(m_pageId)) {
    data.updateImageParams(*params);
  } else {
    ImageSettings::PageParams newParams(BinaryThreshold::otsuThreshold(data.grayImage()), true);

    m_imageSettings->setPageParams(m_pageId, newParams);
    data.updateImageParams(newParams);
  }
}

/*============================ Task::UiUpdater ========================*/

Task::UiUpdater::UiUpdater(std::shared_ptr<Filter> filter,
                           const QImage& image,
                           const ImageId& imageId,
                           const ImageTransformation& xform,
                           const bool batchProcessing)
    : m_filter(std::move(filter)),
      m_image(image),
      m_downscaledImage(ImageView::createDownscaledImage(image)),
      m_imageId(imageId),
      m_xform(xform),
      m_batchProcessing(batchProcessing) {}

void Task::UiUpdater::updateUI(FilterUiInterface* ui) {
  // This function is executed from the GUI thread.
  ui->invalidateThumbnail(PageId(m_imageId));

  if (m_batchProcessing) {
    return;
  }

  OptionsWidget* const optWidget = m_filter->optionsWidget();
  optWidget->postUpdateUI(m_xform.preRotation());
  ui->setOptionsWidget(optWidget, ui->KEEP_OWNERSHIP);

  auto* view = new ImageView(m_image, m_downscaledImage, m_xform);
  ui->setImageWidget(view, ui->TRANSFER_OWNERSHIP);
  QObject::connect(optWidget, SIGNAL(rotated(OrthogonalRotation)), view, SLOT(setPreRotation(OrthogonalRotation)));
}
}  // namespace fix_orientation