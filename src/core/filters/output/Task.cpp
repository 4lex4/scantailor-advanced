// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "Task.h"

#include <DewarpingPointMapper.h>
#include <PolygonUtils.h>
#include <UnitsProvider.h>
#include <core/TiffWriter.h>

#include <QDir>
#include <boost/bind.hpp>
#include <utility>

#include "DebugImagesImpl.h"
#include "DespeckleState.h"
#include "DespeckleView.h"
#include "DespeckleVisualization.h"
#include "DewarpingView.h"
#include "Dpm.h"
#include "ErrorWidget.h"
#include "FillZoneComparator.h"
#include "FillZoneEditor.h"
#include "Filter.h"
#include "FilterData.h"
#include "FilterUiInterface.h"
#include "ImageLoader.h"
#include "ImageView.h"
#include "OptionsWidget.h"
#include "OutputGenerator.h"
#include "OutputImageBuilder.h"
#include "OutputImageWithForeground.h"
#include "OutputImageWithOriginalBackground.h"
#include "PictureZoneComparator.h"
#include "PictureZoneEditor.h"
#include "RenderParams.h"
#include "Settings.h"
#include "TabbedImageView.h"
#include "TaskStatus.h"
#include "ThumbnailPixmapCache.h"
#include "Utils.h"

using namespace imageproc;
using namespace dewarping;

namespace output {
class Task::UiUpdater : public FilterResult {
  Q_DECLARE_TR_FUNCTIONS(output::Task::UiUpdater)
 public:
  UiUpdater(intrusive_ptr<Filter> filter,
            intrusive_ptr<Settings> settings,
            std::unique_ptr<DebugImages> dbgImg,
            const Params& params,
            const ImageTransformation& xform,
            const QRect& virtContentRect,
            const PageId& pageId,
            const QImage& origImage,
            const QImage& outputImage,
            const BinaryImage& pictureMask,
            const DespeckleState& despeckleState,
            const DespeckleVisualization& despeckleVisualization,
            bool batch,
            bool debug);

  void updateUI(FilterUiInterface* ui) override;

  intrusive_ptr<AbstractFilter> filter() override { return m_filter; }

 private:
  intrusive_ptr<Filter> m_filter;
  intrusive_ptr<Settings> m_settings;
  std::unique_ptr<DebugImages> m_dbg;
  Params m_params;
  ImageTransformation m_xform;
  QRect m_virtContentRect;
  PageId m_pageId;
  QImage m_origImage;
  QImage m_downscaledOrigImage;
  QImage m_outputImage;
  QImage m_downscaledOutputImage;
  BinaryImage m_pictureMask;
  DespeckleState m_despeckleState;
  DespeckleVisualization m_despeckleVisualization;
  bool m_batchProcessing;
  bool m_debug;
};


Task::Task(intrusive_ptr<Filter> filter,
           intrusive_ptr<Settings> settings,
           intrusive_ptr<ThumbnailPixmapCache> thumbnailCache,
           const PageId& pageId,
           const OutputFileNameGenerator& outFileNameGen,
           const ImageViewTab lastTab,
           const bool batch,
           const bool debug)
    : m_filter(std::move(filter)),
      m_settings(std::move(settings)),
      m_thumbnailCache(std::move(thumbnailCache)),
      m_pageId(pageId),
      m_outFileNameGen(outFileNameGen),
      m_lastTab(lastTab),
      m_batchProcessing(batch),
      m_debug(debug) {
  if (debug) {
    m_dbg = std::make_unique<DebugImagesImpl>();
  }
}

Task::~Task() = default;

FilterResultPtr Task::process(const TaskStatus& status, const FilterData& data, const QPolygonF& contentRectPhys) {
  status.throwIfCancelled();

  Params params = m_settings->getParams(m_pageId);

  RenderParams renderParams(params.colorParams(), params.splittingOptions());

  ImageTransformation newXform(data.xform());
  newXform.postScaleToDpi(params.outputDpi());

  const QFileInfo sourceFileInfo(m_pageId.imageId().filePath());
  const QString outFilePath(m_outFileNameGen.filePathFor(m_pageId));
  const QFileInfo outFileInfo(outFilePath);
  const QString foregroundDir(Utils::foregroundDir(m_outFileNameGen.outDir()));
  const QString backgroundDir(Utils::backgroundDir(m_outFileNameGen.outDir()));
  const QString originalBackgroundDir(Utils::originalBackgroundDir(m_outFileNameGen.outDir()));
  const QString foregroundFilePath(QDir(foregroundDir).absoluteFilePath(outFileInfo.fileName()));
  const QFileInfo foregroundFileInfo(foregroundFilePath);
  const QString backgroundFilePath(QDir(backgroundDir).absoluteFilePath(outFileInfo.fileName()));
  const QFileInfo backgroundFileInfo(backgroundFilePath);
  const QString originalBackgroundFilePath(QDir(originalBackgroundDir).absoluteFilePath(outFileInfo.fileName()));
  const QFileInfo originalBackgroundFileInfo(originalBackgroundFilePath);

  const QString automaskDir(Utils::automaskDir(m_outFileNameGen.outDir()));
  const QString automaskFilePath(QDir(automaskDir).absoluteFilePath(outFileInfo.fileName()));
  QFileInfo automaskFileInfo(automaskFilePath);

  const QString specklesDir(Utils::specklesDir(m_outFileNameGen.outDir()));
  const QString specklesFilePath(QDir(specklesDir).absoluteFilePath(outFileInfo.fileName()));
  QFileInfo specklesFileInfo(specklesFilePath);

  const bool needPictureEditor = renderParams.mixedOutput() && !m_batchProcessing;
  const bool needSpecklesImage
      = ((params.despeckleLevel() != .0) && renderParams.needBinarization() && !m_batchProcessing);

  {
    std::unique_ptr<OutputParams> storedOutputParams(m_settings->getOutputParams(m_pageId));
    if (storedOutputParams != nullptr) {
      if (storedOutputParams->outputImageParams().getPictureShapeOptions() != params.pictureShapeOptions()) {
        // if picture shape options changed, reset auto picture zones
        OutputProcessingParams outputProcessingParams = m_settings->getOutputProcessingParams(m_pageId);
        outputProcessingParams.setAutoZonesFound(false);
        m_settings->setOutputProcessingParams(m_pageId, outputProcessingParams);
      }
    }
  }

  const OutputGenerator generator(newXform, contentRectPhys);

  OutputImageParams newOutputImageParams(generator.outputImageSize(), generator.outputContentRect(), newXform,
                                         params.outputDpi(), params.colorParams(), params.splittingOptions(),
                                         params.dewarpingOptions(), params.distortionModel(), params.depthPerception(),
                                         params.despeckleLevel(), params.pictureShapeOptions(),
                                         m_settings->getOutputProcessingParams(m_pageId), params.isBlackOnWhite());

  ZoneSet newPictureZones(m_settings->pictureZonesForPage(m_pageId));
  const ZoneSet newFillZones(m_settings->fillZonesForPage(m_pageId));

  bool needReprocess = false;
  do {  // Just to be able to break from it.
    std::unique_ptr<OutputParams> storedOutputParams(m_settings->getOutputParams(m_pageId));

    if (!storedOutputParams) {
      needReprocess = true;
      break;
    }

    if (!storedOutputParams->outputImageParams().matches(newOutputImageParams)) {
      needReprocess = true;
      break;
    }

    if (!PictureZoneComparator::equal(storedOutputParams->pictureZones(), newPictureZones)) {
      needReprocess = true;
      break;
    }

    if (!FillZoneComparator::equal(storedOutputParams->fillZones(), newFillZones)) {
      needReprocess = true;
      break;
    }

    if (!storedOutputParams->sourceFileParams().matches(OutputFileParams(sourceFileInfo))) {
      needReprocess = true;
      break;
    }
    if (!renderParams.splitOutput()) {
      if (!outFileInfo.exists()) {
        needReprocess = true;
        break;
      }
      if (!storedOutputParams->outputFileParams().matches(OutputFileParams(outFileInfo))) {
        needReprocess = true;
        break;
      }
    } else {
      if (!foregroundFileInfo.exists() || !backgroundFileInfo.exists()) {
        needReprocess = true;
        break;
      }
      if (!(storedOutputParams->foregroundFileParams().matches(OutputFileParams(foregroundFileInfo)))
          || !(storedOutputParams->backgroundFileParams().matches(OutputFileParams(backgroundFileInfo)))) {
        needReprocess = true;
        break;
      }
      if (renderParams.originalBackground()) {
        if (!originalBackgroundFileInfo.exists()) {
          needReprocess = true;
          break;
        }
        if (!(storedOutputParams->originalBackgroundFileParams().matches(
                OutputFileParams(originalBackgroundFileInfo)))) {
          needReprocess = true;
          break;
        }
      }
    }

    if (needPictureEditor) {
      if (!automaskFileInfo.exists()) {
        needReprocess = true;
        break;
      }

      if (!storedOutputParams->automaskFileParams().matches(OutputFileParams(automaskFileInfo))) {
        needReprocess = true;
        break;
      }
    }

    if (needSpecklesImage) {
      if (!specklesFileInfo.exists()) {
        needReprocess = true;
        break;
      }
      if (!storedOutputParams->specklesFileParams().matches(OutputFileParams(specklesFileInfo))) {
        needReprocess = true;
        break;
      }
    }
  } while (false);

  QImage outImg;
  BinaryImage automaskImg;
  BinaryImage specklesImg;

  if (!needReprocess) {
    QFile outFile(outFilePath);
    if (outFile.open(QIODevice::ReadOnly)) {
      outImg = ImageLoader::load(outFile, 0);
    }
    if (outImg.isNull() && renderParams.splitOutput()) {
      OutputImageBuilder imageBuilder;
      QFile foregroundFile(foregroundFilePath);
      if (foregroundFile.open(QIODevice::ReadOnly)) {
        imageBuilder.setForegroundImage(ImageLoader::load(foregroundFile, 0));
      }
      QFile backgroundFile(backgroundFilePath);
      if (backgroundFile.open(QIODevice::ReadOnly)) {
        imageBuilder.setBackgroundImage(ImageLoader::load(backgroundFile, 0));
      }
      if (renderParams.originalBackground()) {
        QFile originalBackgroundFile(originalBackgroundFilePath);
        if (originalBackgroundFile.open(QIODevice::ReadOnly)) {
          imageBuilder.setOriginalBackgroundImage(ImageLoader::load(originalBackgroundFile, 0));
        }
      }
      outImg = *imageBuilder.build();
    }
    needReprocess = outImg.isNull();

    if (needPictureEditor && !needReprocess) {
      QFile automaskFile(automaskFilePath);
      if (automaskFile.open(QIODevice::ReadOnly)) {
        automaskImg = BinaryImage(ImageLoader::load(automaskFile, 0));
      }
      needReprocess = automaskImg.isNull() || automaskImg.size() != outImg.size();
    }

    if (needSpecklesImage && !needReprocess) {
      QFile specklesFile(specklesFilePath);
      if (specklesFile.open(QIODevice::ReadOnly)) {
        specklesImg = BinaryImage(ImageLoader::load(specklesFile, 0));
      }
      needReprocess = specklesImg.isNull();
    }
  }

  if (needReprocess) {
    // Even in batch processing mode we should still write automask, because it
    // will be needed when we view the results back in interactive mode.
    // The same applies even more to speckles file, as we need it not only
    // for visualization purposes, but also for re-doing despeckling at
    // different levels without going through the whole output generation process.
    const bool writeAutomask = renderParams.mixedOutput();
    const bool writeSpecklesFile = ((params.despeckleLevel() != .0) && renderParams.needBinarization());

    automaskImg = BinaryImage();
    specklesImg = BinaryImage();

    // OutputGenerator will write a new distortion model
    // there, if dewarping mode is AUTO.
    DistortionModel distortionModel;
    if (params.dewarpingOptions().dewarpingMode() == MANUAL) {
      distortionModel = params.distortionModel();
    }

    bool invalidateParams = false;
    {
      std::unique_ptr<OutputImage> outputImage
          = generator.process(status, data, newPictureZones, newFillZones, distortionModel, params.depthPerception(),
                              writeAutomask ? &automaskImg : nullptr, writeSpecklesFile ? &specklesImg : nullptr,
                              m_dbg.get(), m_pageId, m_settings);

      params = m_settings->getParams(m_pageId);

      if (((params.dewarpingOptions().dewarpingMode() == AUTO)
           || (params.dewarpingOptions().dewarpingMode() == MARGINAL))
          && distortionModel.isValid()) {
        // A new distortion model was generated.
        // We need to save it to be able to modify it manually.
        params.setDistortionModel(distortionModel);
        m_settings->setParams(m_pageId, params);
        newOutputImageParams.setDistortionModel(distortionModel);
      }

      // Saving refreshed params and output processing params.
      newOutputImageParams.setBlackOnWhite(m_settings->getParams(m_pageId).isBlackOnWhite());
      newOutputImageParams.setOutputProcessingParams(m_settings->getOutputProcessingParams(m_pageId));

      if (renderParams.splitOutput()) {
        auto* outputImageWithForeground = dynamic_cast<OutputImageWithForeground*>(outputImage.get());

        QDir().mkdir(foregroundDir);
        QDir().mkdir(backgroundDir);
        if (!TiffWriter::writeImage(foregroundFilePath, outputImageWithForeground->getForegroundImage())
            || !TiffWriter::writeImage(backgroundFilePath, outputImageWithForeground->getBackgroundImage())) {
          invalidateParams = true;
        }

        if (renderParams.originalBackground()) {
          auto* outputImageWithOrigBg = dynamic_cast<OutputImageWithOriginalBackground*>(outputImage.get());

          QDir().mkdir(originalBackgroundDir);
          if (!TiffWriter::writeImage(originalBackgroundFilePath,
                                      outputImageWithOrigBg->getOriginalBackgroundImage())) {
            invalidateParams = true;
          }
        }
      }

      outImg = *outputImage;
    }

    if (!renderParams.originalBackground()) {
      QFile::remove(originalBackgroundFilePath);
    }
    if (!renderParams.splitOutput()) {
      QFile::remove(foregroundFilePath);
      QFile::remove(backgroundFilePath);
    }

    if (!TiffWriter::writeImage(outFilePath, outImg)) {
      invalidateParams = true;
    } else {
      deleteMutuallyExclusiveOutputFiles();
    }

    if (writeSpecklesFile && specklesImg.isNull()) {
      // Even if despeckling didn't actually take place, we still need
      // to write an empty speckles file.  Making it a special case
      // is simply not worth it.
      BinaryImage(outImg.size(), WHITE).swap(specklesImg);
    }

    if (writeAutomask) {
      // Note that QDir::mkdir() will fail if the parent directory,
      // that is $OUT/cache doesn't exist. We want that behaviour,
      // as otherwise when loading a project from a different machine,
      // a whole bunch of bogus directories would be created.
      QDir().mkdir(automaskDir);
      // Also note that QDir::mkdir() will fail if the directory already exists,
      // so we ignore its return value here.
      if (!TiffWriter::writeImage(automaskFilePath, automaskImg.toQImage())) {
        invalidateParams = true;
      }
    }
    if (writeSpecklesFile) {
      if (!QDir().mkpath(specklesDir)) {
        invalidateParams = true;
      } else if (!TiffWriter::writeImage(specklesFilePath, specklesImg.toQImage())) {
        invalidateParams = true;
      }
    }

    if (invalidateParams) {
      m_settings->removeOutputParams(m_pageId);
    } else {
      // Note that we can't reuse *_file_info objects
      // as we've just overwritten those files.
      const OutputParams outParams(
          newOutputImageParams, OutputFileParams(sourceFileInfo), OutputFileParams(QFileInfo(outFilePath)),
          renderParams.splitOutput() ? OutputFileParams(QFileInfo(foregroundFilePath)) : OutputFileParams(),
          renderParams.splitOutput() ? OutputFileParams(QFileInfo(backgroundFilePath)) : OutputFileParams(),
          renderParams.originalBackground() ? OutputFileParams(QFileInfo(originalBackgroundFilePath))
                                            : OutputFileParams(),
          writeAutomask ? OutputFileParams(QFileInfo(automaskFilePath)) : OutputFileParams(),
          writeSpecklesFile ? OutputFileParams(QFileInfo(specklesFilePath)) : OutputFileParams(), newPictureZones,
          newFillZones);

      m_settings->setOutputParams(m_pageId, outParams);
    }

    m_thumbnailCache->recreateThumbnail(ImageId(outFilePath), outImg);
  }

  const DespeckleState despeckleState(outImg, specklesImg, params.despeckleLevel(), params.outputDpi());

  DespeckleVisualization despeckleVisualization;
  if (m_lastTab == TAB_DESPECKLING) {
    // Because constructing DespeckleVisualization takes a noticeable
    // amount of time, we only do it if we are sure we'll need it.
    // Otherwise it will get constructed on demand.
    despeckleVisualization = despeckleState.visualize();
  }
  return make_intrusive<UiUpdater>(m_filter, m_settings, std::move(m_dbg), params, newXform,
                                   generator.outputContentRect(), m_pageId, data.origImage(), outImg, automaskImg,
                                   despeckleState, despeckleVisualization, m_batchProcessing, m_debug);
}  // Task::process

/**
 * Delete output files mutually exclusive to m_pageId.
 */
void Task::deleteMutuallyExclusiveOutputFiles() {
  switch (m_pageId.subPage()) {
    case PageId::SINGLE_PAGE:
      QFile::remove(m_outFileNameGen.filePathFor(PageId(m_pageId.imageId(), PageId::LEFT_PAGE)));
      QFile::remove(m_outFileNameGen.filePathFor(PageId(m_pageId.imageId(), PageId::RIGHT_PAGE)));
      break;
    case PageId::LEFT_PAGE:
    case PageId::RIGHT_PAGE:
      QFile::remove(m_outFileNameGen.filePathFor(PageId(m_pageId.imageId(), PageId::SINGLE_PAGE)));
      break;
  }
}

/*============================ Task::UiUpdater ==========================*/

Task::UiUpdater::UiUpdater(intrusive_ptr<Filter> filter,
                           intrusive_ptr<Settings> settings,
                           std::unique_ptr<DebugImages> dbgImg,
                           const Params& params,
                           const ImageTransformation& xform,
                           const QRect& virtContentRect,
                           const PageId& pageId,
                           const QImage& origImage,
                           const QImage& outputImage,
                           const BinaryImage& pictureMask,
                           const DespeckleState& despeckleState,
                           const DespeckleVisualization& despeckleVisualization,
                           const bool batch,
                           const bool debug)
    : m_filter(std::move(filter)),
      m_settings(std::move(settings)),
      m_dbg(std::move(dbgImg)),
      m_params(params),
      m_xform(xform),
      m_virtContentRect(virtContentRect),
      m_pageId(pageId),
      m_origImage(origImage),
      m_downscaledOrigImage(ImageView::createDownscaledImage(origImage)),
      m_outputImage(outputImage),
      m_downscaledOutputImage(ImageView::createDownscaledImage(outputImage)),
      m_pictureMask(pictureMask),
      m_despeckleState(despeckleState),
      m_despeckleVisualization(despeckleVisualization),
      m_batchProcessing(batch),
      m_debug(debug) {}

void Task::UiUpdater::updateUI(FilterUiInterface* ui) {
  // This function is executed from the GUI thread.
  ui->invalidateThumbnail(m_pageId);

  if (m_batchProcessing) {
    return;
  }

  OptionsWidget* const optWidget = m_filter->optionsWidget();
  optWidget->postUpdateUI();
  ui->setOptionsWidget(optWidget, ui->KEEP_OWNERSHIP);

  auto tabImageRectMap = std::make_unique<std::unordered_map<ImageViewTab, QRectF, std::hash<int>>>();

  auto imageView = std::make_unique<ImageView>(m_outputImage, m_downscaledOutputImage);
  const QPixmap downscaledOutputPixmap(imageView->downscaledPixmap());
  tabImageRectMap->insert(std::pair<ImageViewTab, QRectF>(TAB_OUTPUT, m_xform.resultingRect()));

  auto dewarpingView = std::make_unique<DewarpingView>(
      m_origImage, m_downscaledOrigImage, m_xform.transform(),
      PolygonUtils::convexHull(m_xform.resultingPreCropArea().toStdVector()), m_virtContentRect, m_pageId,
      m_params.dewarpingOptions(), m_params.distortionModel(), optWidget->depthPerception());
  const QPixmap downscaledOrigPixmap(dewarpingView->downscaledPixmap());
  QObject::connect(optWidget, SIGNAL(depthPerceptionChanged(double)), dewarpingView.get(),
                   SLOT(depthPerceptionChanged(double)));
  QObject::connect(dewarpingView.get(), SIGNAL(distortionModelChanged(const dewarping::DistortionModel&)), optWidget,
                   SLOT(distortionModelChanged(const dewarping::DistortionModel&)));
  tabImageRectMap->insert(
      std::pair<ImageViewTab, QRectF>(TAB_DEWARPING, m_xform.resultingPreCropArea().boundingRect()));

  std::unique_ptr<QWidget> pictureZoneEditor;
  if (m_pictureMask.isNull()) {
    pictureZoneEditor = std::make_unique<ErrorWidget>(tr("Picture zones are only available in Mixed mode."));
  } else {
    pictureZoneEditor = std::make_unique<output::PictureZoneEditor>(m_origImage, downscaledOrigPixmap, m_pictureMask,
                                                                    m_xform.transform(), m_xform.resultingPreCropArea(),
                                                                    m_pageId, m_settings);
    QObject::connect(pictureZoneEditor.get(), SIGNAL(invalidateThumbnail(const PageId&)), optWidget,
                     SIGNAL(invalidateThumbnail(const PageId&)));
    tabImageRectMap->insert(
        std::pair<ImageViewTab, QRectF>(TAB_PICTURE_ZONES, m_xform.resultingPreCropArea().boundingRect()));
  }

  // We make sure we never need to update the original <-> output
  // mapping at run time, that is without reloading.
  // In OptionsWidget::dewarpingChanged() we make sure to reload
  // if we are on the "Fill Zones" tab, and if not, it will be reloaded
  // anyway when another tab is selected.
  boost::function<QPointF(const QPointF&)> origToOutput;
  boost::function<QPointF(const QPointF&)> outputToOrig;
  if ((m_params.dewarpingOptions().dewarpingMode() != OFF) && m_params.distortionModel().isValid()) {
    const QTransform rotateXform
        = Utils::rotate(m_params.dewarpingOptions().getPostDeskewAngle(), m_xform.resultingRect().toRect());
    auto mapper = std::make_shared<DewarpingPointMapper>(m_params.distortionModel(), m_params.depthPerception().value(),
                                                         m_xform.transform(), m_virtContentRect, rotateXform);
    origToOutput = boost::bind(&DewarpingPointMapper::mapToDewarpedSpace, mapper, _1);
    outputToOrig = boost::bind(&DewarpingPointMapper::mapToWarpedSpace, mapper, _1);
  } else {
    using MapPointFunc = QPointF (QTransform::*)(const QPointF&) const;
    origToOutput = boost::bind((MapPointFunc) &QTransform::map, m_xform.transform(), _1);
    outputToOrig = boost::bind((MapPointFunc) &QTransform::map, m_xform.transformBack(), _1);
  }

  auto fillZoneEditor = std::make_unique<FillZoneEditor>(m_outputImage, downscaledOutputPixmap, origToOutput,
                                                         outputToOrig, m_pageId, m_settings);
  QObject::connect(fillZoneEditor.get(), SIGNAL(invalidateThumbnail(const PageId&)), optWidget,
                   SIGNAL(invalidateThumbnail(const PageId&)));
  tabImageRectMap->insert(std::pair<ImageViewTab, QRectF>(TAB_FILL_ZONES, m_xform.resultingRect()));

  std::unique_ptr<QWidget> despeckleView;
  if (m_params.colorParams().colorMode() == COLOR_GRAYSCALE) {
    despeckleView = std::make_unique<ErrorWidget>(tr("Despeckling can't be done in Color / Grayscale mode."));
  } else {
    despeckleView = std::make_unique<output::DespeckleView>(m_despeckleState, m_despeckleVisualization, m_debug);
    QObject::connect(optWidget, SIGNAL(despeckleLevelChanged(double, bool*)), despeckleView.get(),
                     SLOT(despeckleLevelChanged(double, bool*)));
    tabImageRectMap->insert(std::pair<ImageViewTab, QRectF>(TAB_DESPECKLING, m_xform.resultingRect()));
  }

  auto tabWidget = std::make_unique<TabbedImageView>();
  tabWidget->setDocumentMode(true);
  tabWidget->setTabPosition(QTabWidget::East);
  tabWidget->addTab(imageView.release(), tr("Output"), TAB_OUTPUT);
  tabWidget->addTab(pictureZoneEditor.release(), tr("Picture Zones"), TAB_PICTURE_ZONES);
  tabWidget->addTab(fillZoneEditor.release(), tr("Fill Zones"), TAB_FILL_ZONES);
  tabWidget->addTab(dewarpingView.release(), tr("Dewarping"), TAB_DEWARPING);
  tabWidget->addTab(despeckleView.release(), tr("Despeckling"), TAB_DESPECKLING);
  tabWidget->setCurrentTab(optWidget->lastTab());
  tabWidget->setImageRectMap(std::move(tabImageRectMap));

  QObject::connect(tabWidget.get(), SIGNAL(tabChanged(ImageViewTab)), optWidget, SLOT(tabChanged(ImageViewTab)));

  ui->setImageWidget(tabWidget.release(), ui->TRANSFER_OWNERSHIP, m_dbg.get());
}  // Task::UiUpdater::updateUI
}  // namespace output