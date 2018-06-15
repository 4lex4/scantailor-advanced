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

#include <BlackOnWhiteEstimator.h>
#include <imageproc/Grayscale.h>
#include <imageproc/PolygonRasterizer.h>
#include <QSettings>
#include <utility>
#include "DebugImages.h"
#include "Dpm.h"
#include "Filter.h"
#include "FilterData.h"
#include "FilterUiInterface.h"
#include "ImageView.h"
#include "OptionsWidget.h"
#include "Task.h"
#include "TaskStatus.h"
#include "filters/select_content/Task.h"
#include "imageproc/BinaryImage.h"
#include "imageproc/Morphology.h"
#include "imageproc/OrthogonalRotation.h"
#include "imageproc/RasterOp.h"
#include "imageproc/ReduceThreshold.h"
#include "imageproc/SeedFill.h"
#include "imageproc/SkewFinder.h"
#include "imageproc/UpscaleIntegerTimes.h"

namespace deskew {
using namespace imageproc;

class Task::UiUpdater : public FilterResult {
 public:
  UiUpdater(intrusive_ptr<Filter> filter,
            std::unique_ptr<DebugImages> dbg_img,
            const QImage& image,
            const PageId& page_id,
            const ImageTransformation& xform,
            const OptionsWidget::UiData& ui_data,
            bool batch_processing);

  void updateUI(FilterUiInterface* ui) override;

  intrusive_ptr<AbstractFilter> filter() override { return m_ptrFilter; }

 private:
  intrusive_ptr<Filter> m_ptrFilter;
  std::unique_ptr<DebugImages> m_ptrDbg;
  QImage m_image;
  QImage m_downscaledImage;
  PageId m_pageId;
  ImageTransformation m_xform;
  OptionsWidget::UiData m_uiData;
  bool m_batchProcessing;
};


Task::Task(intrusive_ptr<Filter> filter,
           intrusive_ptr<Settings> settings,
           intrusive_ptr<ImageSettings> image_settings,
           intrusive_ptr<select_content::Task> next_task,
           const PageId& page_id,
           const bool batch_processing,
           const bool debug)
    : m_ptrFilter(std::move(filter)),
      m_ptrSettings(std::move(settings)),
      m_ptrImageSettings(std::move(image_settings)),
      m_ptrNextTask(std::move(next_task)),
      m_pageId(page_id),
      m_batchProcessing(batch_processing) {
  if (debug) {
    m_ptrDbg = std::make_unique<DebugImages>();
  }
}

Task::~Task() = default;

FilterResultPtr Task::process(const TaskStatus& status, FilterData data) {
  status.throwIfCancelled();

  const Dependencies deps(data.xform().preCropArea(), data.xform().preRotation());

  std::unique_ptr<Params> params(m_ptrSettings->getPageParams(m_pageId));
  updateFilterData(status, data, (!params || !deps.matches(params->dependencies())));

  OptionsWidget::UiData ui_data;
  ui_data.setDependencies(deps);

  if (params) {
    if ((!deps.matches(params->dependencies()) || (params->deskewAngle() != ui_data.effectiveDeskewAngle()))
        && (params->mode() == MODE_AUTO)) {
      params.reset();
    } else {
      ui_data.setEffectiveDeskewAngle(params->deskewAngle());
      ui_data.setMode(params->mode());

      Params new_params(ui_data.effectiveDeskewAngle(), deps, ui_data.mode());
      m_ptrSettings->setPageParams(m_pageId, new_params);
    }
  }

  if (!params) {
    const QRectF image_area(data.xform().transformBack().mapRect(data.xform().resultingRect()));
    const QRect bounded_image_area(image_area.toRect().intersected(data.origImage().rect()));

    status.throwIfCancelled();

    if (bounded_image_area.isValid()) {
      BinaryImage rotated_image(orthogonalRotation(
          BinaryImage(data.isBlackOnWhite() ? data.grayImage() : data.grayImage().inverted(), bounded_image_area,
                      data.isBlackOnWhite() ? data.bwThreshold() : BinaryThreshold(256 - int(data.bwThreshold()))),
          data.xform().preRotation().toDegrees()));
      if (m_ptrDbg) {
        m_ptrDbg->add(rotated_image, "bw_rotated");
      }

      const QSize unrotated_dpm(Dpm(data.origImage()).toSize());
      const Dpm rotated_dpm(data.xform().preRotation().rotate(unrotated_dpm));
      cleanup(status, rotated_image, Dpi(rotated_dpm));
      if (m_ptrDbg) {
        m_ptrDbg->add(rotated_image, "after_cleanup");
      }

      status.throwIfCancelled();

      SkewFinder skew_finder;
      skew_finder.setResolutionRatio((double) rotated_dpm.horizontal() / rotated_dpm.vertical());
      const Skew skew(skew_finder.findSkew(rotated_image));

      if (skew.confidence() >= skew.GOOD_CONFIDENCE) {
        ui_data.setEffectiveDeskewAngle(-skew.angle());
      } else {
        ui_data.setEffectiveDeskewAngle(0);
      }
      ui_data.setMode(MODE_AUTO);

      Params new_params(ui_data.effectiveDeskewAngle(), deps, ui_data.mode());
      m_ptrSettings->setPageParams(m_pageId, new_params);

      status.throwIfCancelled();
    }
  }

  ImageTransformation new_xform(data.xform());
  new_xform.setPostRotation(ui_data.effectiveDeskewAngle());

  if (m_ptrNextTask) {
    return m_ptrNextTask->process(status, FilterData(data, new_xform));
  } else {
    return make_intrusive<UiUpdater>(m_ptrFilter, std::move(m_ptrDbg), data.origImage(), m_pageId, new_xform, ui_data,
                                     m_batchProcessing);
  }
}  // Task::process

void Task::cleanup(const TaskStatus& status, BinaryImage& image, const Dpi& dpi) {
  // We don't have to clean up every piece of garbage.
  // The only concern are the horizontal shadows, which we remove here.

  Dpi reduced_dpi(dpi);
  BinaryImage reduced_image;

  {
    ReduceThreshold reductor(image);
    while (reduced_dpi.horizontal() >= 200 && reduced_dpi.vertical() >= 200) {
      reductor.reduce(2);
      reduced_dpi = Dpi(reduced_dpi.horizontal() / 2, reduced_dpi.vertical() / 2);
    }
    reduced_image = reductor.image();
  }

  status.throwIfCancelled();

  const QSize brick(from150dpi(QSize(200, 14), reduced_dpi));
  BinaryImage opened(openBrick(reduced_image, brick, BLACK));
  reduced_image.release();

  status.throwIfCancelled();

  BinaryImage seed(upscaleIntegerTimes(opened, image.size(), WHITE));
  opened.release();

  status.throwIfCancelled();

  BinaryImage garbage(seedFill(seed, image, CONN8));
  seed.release();

  status.throwIfCancelled();

  rasterOp<RopSubtract<RopDst, RopSrc>>(image, garbage);
}  // Task::cleanup

int Task::from150dpi(int size, int target_dpi) {
  const int new_size = (size * target_dpi + 75) / 150;
  if (new_size < 1) {
    return 1;
  }

  return new_size;
}

QSize Task::from150dpi(const QSize& size, const Dpi& target_dpi) {
  const int width = from150dpi(size.width(), target_dpi.horizontal());
  const int height = from150dpi(size.height(), target_dpi.vertical());

  return QSize(width, height);
}

void Task::updateFilterData(const TaskStatus& status, FilterData& data, bool needUpdate) {
  const std::unique_ptr<ImageSettings::PageParams> params = m_ptrImageSettings->getPageParams(m_pageId);
  if (!needUpdate && params) {
    data.updateImageParams(*params);
  } else {
    const GrayImage& img = data.grayImage();
    BinaryImage mask(img.size(), BLACK);
    PolygonRasterizer::fillExcept(mask, WHITE, data.xform().resultingPreCropArea(), Qt::WindingFill);
    bool isBlackOnWhite = true;
    if (QSettings().value("settings/blackOnWhiteDetection", true).toBool()) {
      isBlackOnWhite = BlackOnWhiteEstimator::isBlackOnWhite(data.grayImage(), data.xform(), status, m_ptrDbg.get());
    }
    ImageSettings::PageParams new_params(BinaryThreshold::otsuThreshold(GrayscaleHistogram(img, mask)), isBlackOnWhite);

    m_ptrImageSettings->setPageParams(m_pageId, new_params);
    data.updateImageParams(new_params);
  }
}

/*============================ Task::UiUpdater ==========================*/

Task::UiUpdater::UiUpdater(intrusive_ptr<Filter> filter,
                           std::unique_ptr<DebugImages> dbg_img,
                           const QImage& image,
                           const PageId& page_id,
                           const ImageTransformation& xform,
                           const OptionsWidget::UiData& ui_data,
                           const bool batch_processing)
    : m_ptrFilter(std::move(filter)),
      m_ptrDbg(std::move(dbg_img)),
      m_image(image),
      m_downscaledImage(ImageView::createDownscaledImage(image)),
      m_pageId(page_id),
      m_xform(xform),
      m_uiData(ui_data),
      m_batchProcessing(batch_processing) {}

void Task::UiUpdater::updateUI(FilterUiInterface* ui) {
  // This function is executed from the GUI thread.
  OptionsWidget* const opt_widget = m_ptrFilter->optionsWidget();
  opt_widget->postUpdateUI(m_uiData);
  ui->setOptionsWidget(opt_widget, ui->KEEP_OWNERSHIP);

  ui->invalidateThumbnail(m_pageId);

  if (m_batchProcessing) {
    return;
  }

  auto* view = new ImageView(m_image, m_downscaledImage, m_xform);
  ui->setImageWidget(view, ui->TRANSFER_OWNERSHIP, m_ptrDbg.get());

  QObject::connect(view, SIGNAL(manualDeskewAngleSet(double)), opt_widget,
                   SLOT(manualDeskewAngleSetExternally(double)));
  QObject::connect(opt_widget, SIGNAL(manualDeskewAngleSet(double)), view,
                   SLOT(manualDeskewAngleSetExternally(double)));
}
}  // namespace deskew
