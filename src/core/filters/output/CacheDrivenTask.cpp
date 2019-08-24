// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "CacheDrivenTask.h"
#include <QDir>
#include <QFileInfo>
#include <utility>
#include "FillZoneComparator.h"
#include "IncompleteThumbnail.h"
#include "OutputGenerator.h"
#include "OutputParams.h"
#include "PageInfo.h"
#include "Params.h"
#include "PictureZoneComparator.h"
#include "RenderParams.h"
#include "Settings.h"
#include "Thumbnail.h"
#include "Utils.h"
#include "core/AbstractFilterDataCollector.h"
#include "core/ThumbnailCollector.h"

namespace output {
CacheDrivenTask::CacheDrivenTask(intrusive_ptr<Settings> settings, const OutputFileNameGenerator& outFileNameGen)
    : m_settings(std::move(settings)), m_outFileNameGen(outFileNameGen) {}

CacheDrivenTask::~CacheDrivenTask() = default;

void CacheDrivenTask::process(const PageInfo& pageInfo,
                              AbstractFilterDataCollector* collector,
                              const ImageTransformation& xform,
                              const QPolygonF& contentRectPhys) {
  if (auto* thumbCol = dynamic_cast<ThumbnailCollector*>(collector)) {
    const QString outFilePath(m_outFileNameGen.filePathFor(pageInfo.id()));
    const QFileInfo outFileInfo(outFilePath);
    const QString foregroundDir(Utils::foregroundDir(m_outFileNameGen.outDir()));
    const QString backgroundDir(Utils::backgroundDir(m_outFileNameGen.outDir()));
    const QString originalBackgroundDir(Utils::originalBackgroundDir(m_outFileNameGen.outDir()));
    const QString foregroundFilePath(QDir(foregroundDir).absoluteFilePath(outFileInfo.fileName()));
    const QString backgroundFilePath(QDir(backgroundDir).absoluteFilePath(outFileInfo.fileName()));
    const QString originalBackgroundFilePath(QDir(originalBackgroundDir).absoluteFilePath(outFileInfo.fileName()));
    const QFileInfo foregroundFileInfo(foregroundFilePath);
    const QFileInfo backgroundFileInfo(backgroundFilePath);
    const QFileInfo originalBackgroundFileInfo(originalBackgroundFilePath);

    const Params params(m_settings->getParams(pageInfo.id()));
    RenderParams renderParams(params.colorParams(), params.splittingOptions());

    ImageTransformation newXform(xform);
    newXform.postScaleToDpi(params.outputDpi());

    bool needReprocess = false;

    do {  // Just to be able to break from it.
      std::unique_ptr<OutputParams> storedOutputParams(m_settings->getOutputParams(pageInfo.id()));

      if (!storedOutputParams) {
        needReprocess = true;
        break;
      }

      const OutputGenerator generator(newXform, contentRectPhys);
      const OutputImageParams newOutputImageParams(
          generator.outputImageSize(), generator.outputContentRect(), newXform, params.outputDpi(),
          params.colorParams(), params.splittingOptions(), params.dewarpingOptions(), params.distortionModel(),
          params.depthPerception(), params.despeckleLevel(), params.pictureShapeOptions(),
          m_settings->getOutputProcessingParams(pageInfo.id()), params.isBlackOnWhite());

      if (!storedOutputParams->outputImageParams().matches(newOutputImageParams)) {
        needReprocess = true;
        break;
      }

      const ZoneSet newPictureZones(m_settings->pictureZonesForPage(pageInfo.id()));
      if (!PictureZoneComparator::equal(storedOutputParams->pictureZones(), newPictureZones)) {
        needReprocess = true;
        break;
      }

      const ZoneSet newFillZones(m_settings->fillZonesForPage(pageInfo.id()));
      if (!FillZoneComparator::equal(storedOutputParams->fillZones(), newFillZones)) {
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
    } while (false);

    if (needReprocess) {
      thumbCol->processThumbnail(std::unique_ptr<QGraphicsItem>(new IncompleteThumbnail(
          thumbCol->thumbnailCache(), thumbCol->maxLogicalThumbSize(), pageInfo.imageId(), newXform)));
    } else {
      const ImageTransformation outXform(newXform.resultingRect(), params.outputDpi());

      thumbCol->processThumbnail(std::unique_ptr<QGraphicsItem>(
          new Thumbnail(thumbCol->thumbnailCache(), thumbCol->maxLogicalThumbSize(), ImageId(outFilePath), outXform)));
    }
  }
}  // CacheDrivenTask::process
}  // namespace output