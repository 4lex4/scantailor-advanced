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
CacheDrivenTask::CacheDrivenTask(intrusive_ptr<Settings> settings, const OutputFileNameGenerator& out_file_name_gen)
    : m_settings(std::move(settings)), m_outFileNameGen(out_file_name_gen) {}

CacheDrivenTask::~CacheDrivenTask() = default;

void CacheDrivenTask::process(const PageInfo& page_info,
                              AbstractFilterDataCollector* collector,
                              const ImageTransformation& xform,
                              const QPolygonF& content_rect_phys) {
  if (auto* thumb_col = dynamic_cast<ThumbnailCollector*>(collector)) {
    const QString out_file_path(m_outFileNameGen.filePathFor(page_info.id()));
    const QFileInfo out_file_info(out_file_path);
    const QString foreground_dir(Utils::foregroundDir(m_outFileNameGen.outDir()));
    const QString background_dir(Utils::backgroundDir(m_outFileNameGen.outDir()));
    const QString original_background_dir(Utils::originalBackgroundDir(m_outFileNameGen.outDir()));
    const QString foreground_file_path(QDir(foreground_dir).absoluteFilePath(out_file_info.fileName()));
    const QString background_file_path(QDir(background_dir).absoluteFilePath(out_file_info.fileName()));
    const QString original_background_file_path(
        QDir(original_background_dir).absoluteFilePath(out_file_info.fileName()));
    const QFileInfo foreground_file_info(foreground_file_path);
    const QFileInfo background_file_info(background_file_path);
    const QFileInfo original_background_file_info(original_background_file_path);

    const Params params(m_settings->getParams(page_info.id()));
    RenderParams render_params(params.colorParams(), params.splittingOptions());

    ImageTransformation new_xform(xform);
    new_xform.postScaleToDpi(params.outputDpi());

    bool need_reprocess = false;

    do {  // Just to be able to break from it.
      std::unique_ptr<OutputParams> stored_output_params(m_settings->getOutputParams(page_info.id()));

      if (!stored_output_params) {
        need_reprocess = true;
        break;
      }

      const OutputGenerator generator(new_xform, content_rect_phys);
      const OutputImageParams new_output_image_params(
          generator.outputImageSize(), generator.outputContentRect(), new_xform, params.outputDpi(),
          params.colorParams(), params.splittingOptions(), params.dewarpingOptions(), params.distortionModel(),
          params.depthPerception(), params.despeckleLevel(), params.pictureShapeOptions(),
          m_settings->getOutputProcessingParams(page_info.id()), params.isBlackOnWhite());

      if (!stored_output_params->outputImageParams().matches(new_output_image_params)) {
        need_reprocess = true;
        break;
      }

      const ZoneSet new_picture_zones(m_settings->pictureZonesForPage(page_info.id()));
      if (!PictureZoneComparator::equal(stored_output_params->pictureZones(), new_picture_zones)) {
        need_reprocess = true;
        break;
      }

      const ZoneSet new_fill_zones(m_settings->fillZonesForPage(page_info.id()));
      if (!FillZoneComparator::equal(stored_output_params->fillZones(), new_fill_zones)) {
        need_reprocess = true;
        break;
      }

      if (!render_params.splitOutput()) {
        if (!out_file_info.exists()) {
          need_reprocess = true;
          break;
        }

        if (!stored_output_params->outputFileParams().matches(OutputFileParams(out_file_info))) {
          need_reprocess = true;
          break;
        }
      } else {
        if (!foreground_file_info.exists() || !background_file_info.exists()) {
          need_reprocess = true;
          break;
        }
        if (!(stored_output_params->foregroundFileParams().matches(OutputFileParams(foreground_file_info)))
            || !(stored_output_params->backgroundFileParams().matches(OutputFileParams(background_file_info)))) {
          need_reprocess = true;
          break;
        }

        if (render_params.originalBackground()) {
          if (!original_background_file_info.exists()) {
            need_reprocess = true;
            break;
          }
          if (!(stored_output_params->originalBackgroundFileParams().matches(
                  OutputFileParams(original_background_file_info)))) {
            need_reprocess = true;
            break;
          }
        }
      }
    } while (false);

    if (need_reprocess) {
      thumb_col->processThumbnail(std::unique_ptr<QGraphicsItem>(new IncompleteThumbnail(
          thumb_col->thumbnailCache(), thumb_col->maxLogicalThumbSize(), page_info.imageId(), new_xform)));
    } else {
      const ImageTransformation out_xform(new_xform.resultingRect(), params.outputDpi());

      thumb_col->processThumbnail(std::unique_ptr<QGraphicsItem>(new Thumbnail(
          thumb_col->thumbnailCache(), thumb_col->maxLogicalThumbSize(), ImageId(out_file_path), out_xform)));
    }
  }
}  // CacheDrivenTask::process
}  // namespace output