// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef OUTPUT_OUTPUTGENERATOR_H_
#define OUTPUT_OUTPUTGENERATOR_H_

#include <QRect>
#include <memory>
#include "ImageTransformation.h"
#include "OutputImage.h"
#include "intrusive_ptr.h"

class TaskStatus;
class DebugImages;
class FilterData;
class ZoneSet;
class QSize;
class QImage;
class PageId;

namespace imageproc {
class BinaryImage;
class BinaryThreshold;
class GrayImage;
}  // namespace imageproc

namespace dewarping {
class DistortionModel;
class CylindricalSurfaceDewarper;
}  // namespace dewarping

namespace output {
class Settings;
class DepthPerception;

class OutputGenerator {
 public:
  OutputGenerator(const ImageTransformation& xform, const QPolygonF& content_rect_phys);

  /**
   * \brief Produce the output image.
   *
   * \param status For asynchronous task cancellation.
   * \param input The input image plus data produced by previous stages.
   * \param picture_zones A set of manual picture zones.
   * \param fill_zones A set of manual fill zones.
   * \param distortion_model A curved rectangle.
   * \param auto_picture_mask If provided, the auto-detected picture mask
   *        will be written there.  It would only happen if automatic picture
   *        detection actually took place.  Otherwise, nothing will be
   *        written into the provided image.  Black areas on the mask
   *        indicate pictures.  The manual zones aren't represented in it.
   * \param speckles_image If provided, the speckles removed from the
   *        binarized image will be written there.  It would only happen
   *        if despeckling was required and actually took place.
   *        Otherwise, nothing will be written into the provided image.
   *        Because despeckling is intentionally the last operation on
   *        the B/W part of the image, the "pre-despeckle" image may be
   *        restored from the output and speckles images, allowing despeckling
   *        to be performed again with different settings, without going
   *        through the whole output generation process again.
   * \param dbg An optional sink for debugging images.
   */
  std::unique_ptr<OutputImage> process(const TaskStatus& status,
                                       const FilterData& input,
                                       ZoneSet& picture_zones,
                                       const ZoneSet& fill_zones,
                                       dewarping::DistortionModel& distortion_model,
                                       const DepthPerception& depth_perception,
                                       imageproc::BinaryImage* auto_picture_mask,
                                       imageproc::BinaryImage* speckles_image,
                                       DebugImages* dbg,
                                       const PageId& pageId,
                                       const intrusive_ptr<Settings>& settings) const;

  QSize outputImageSize() const;

  /**
   * \brief Returns the content rectangle in output image coordinates.
   */
  QRect outputContentRect() const;

 private:
  class Processor;

  /**
   * Transformation from the input to the output image coordinates.
   */
  ImageTransformation m_xform;

  /**
   * The rectangle corresponding to the output image.
   * The top-left corner will always be at (0, 0).
   */
  QRect m_outRect;

  /**
   * The content rectangle in output image coordinates.
   */
  QRect m_contentRect;
};
}  // namespace output
#endif  // ifndef OUTPUT_OUTPUTGENERATOR_H_
