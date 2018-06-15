/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2007-2008  Joseph Artsimovich <joseph_a@mail.ru>

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

  intrusive_ptr<AbstractFilter> filter() override { return m_ptrFilter; }

 private:
  intrusive_ptr<Filter> m_ptrFilter;
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
    : m_ptrFilter(std::move(filter)),
      m_ptrNextTask(std::move(next_task)),
      m_ptrSettings(std::move(settings)),
      m_ptrImageSettings(std::move(image_settings)),
      m_pageId(page_id),
      m_imageId(m_pageId.imageId()),
      m_batchProcessing(batch_processing) {}

Task::~Task() = default;

FilterResultPtr Task::process(const TaskStatus& status, FilterData data) {
  // This function is executed from the worker thread.

  status.throwIfCancelled();

  updateFilterData(data);

  ImageTransformation xform(data.xform());
  xform.setPreRotation(m_ptrSettings->getRotationFor(m_imageId));

  if (m_ptrNextTask) {
    return m_ptrNextTask->process(status, FilterData(data, xform));
  } else {
    return make_intrusive<UiUpdater>(m_ptrFilter, data.origImage(), m_imageId, xform, m_batchProcessing);
  }
}

void Task::updateFilterData(FilterData& data) {
  if (const std::unique_ptr<ImageSettings::PageParams> params = m_ptrImageSettings->getPageParams(m_pageId)) {
    data.updateImageParams(*params);
  } else {
    ImageSettings::PageParams new_params(BinaryThreshold::otsuThreshold(data.grayImage()), true);

    m_ptrImageSettings->setPageParams(m_pageId, new_params);
    data.updateImageParams(new_params);
  }
}

/*============================ Task::UiUpdater ========================*/

Task::UiUpdater::UiUpdater(intrusive_ptr<Filter> filter,
                           const QImage& image,
                           const ImageId& image_id,
                           const ImageTransformation& xform,
                           const bool batch_processing)
    : m_ptrFilter(std::move(filter)),
      m_image(image),
      m_downscaledImage(ImageView::createDownscaledImage(image)),
      m_imageId(image_id),
      m_xform(xform),
      m_batchProcessing(batch_processing) {}

void Task::UiUpdater::updateUI(FilterUiInterface* ui) {
  // This function is executed from the GUI thread.
  OptionsWidget* const opt_widget = m_ptrFilter->optionsWidget();
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