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

#ifndef OUTPUT_OUTPUTGENERATOR_H_
#define OUTPUT_OUTPUTGENERATOR_H_

#include <QtCore/qmath.h>
#include <QColor>
#include <QFile>
#include <QLineF>
#include <QMessageBox>
#include <QPointF>
#include <QPolygonF>
#include <QRect>
#include <QSize>
#include <QTransform>
#include <boost/function.hpp>
#include <cstdint>
#include <utility>
#include <vector>
#include "ColorParams.h"
#include "DepthPerception.h"
#include "DespeckleLevel.h"
#include "DewarpingOptions.h"
#include "Dpi.h"
#include "ImageTransformation.h"
#include "OutputProcessingParams.h"
#include "PageId.h"
#include "Params.h"
#include "Settings.h"
#include "SplitImage.h"
#include "TiffWriter.h"
#include "imageproc/Connectivity.h"
#include "imageproc/SkewFinder.h"
#include "intrusive_ptr.h"

class TaskStatus;
class DebugImages;
class FilterData;
class ZoneSet;
class QSize;
class QImage;

namespace imageproc {
class BinaryImage;
class BinaryThreshold;

class GrayImage;
}  // namespace imageproc

namespace dewarping {
class DistortionModel;
class CylindricalSurfaceDewarper;
}  // namespace dewarping

using namespace imageproc;

namespace output {
class OutputGenerator {
 public:
  OutputGenerator(const Dpi& dpi,
                  const ColorParams& color_params,
                  const SplittingOptions& splitting_options,
                  const PictureShapeOptions& picture_shape_options,
                  const DewarpingOptions& dewarping_options,
                  const OutputProcessingParams& output_processing_params,
                  double despeckle_level,
                  const ImageTransformation& xform,
                  const QPolygonF& content_rect_phys);

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
  QImage process(const TaskStatus& status,
                 const FilterData& input,
                 ZoneSet& picture_zones,
                 const ZoneSet& fill_zones,
                 dewarping::DistortionModel& distortion_model,
                 const DepthPerception& depth_perception,
                 imageproc::BinaryImage* auto_picture_mask,
                 imageproc::BinaryImage* speckles_image,
                 DebugImages* dbg,
                 const PageId& p_pageId,
                 const intrusive_ptr<Settings>& p_settings,
                 SplitImage* splitImage);

  QSize outputImageSize() const;

  /**
   * \brief Returns the content rectangle in output image coordinates.
   */
  QRect outputContentRect() const;

  const QTransform& getPostTransform() const;

 private:
  QImage processImpl(const TaskStatus& status,
                     const FilterData& input,
                     ZoneSet& picture_zones,
                     const ZoneSet& fill_zones,
                     dewarping::DistortionModel& distortion_model,
                     const DepthPerception& depth_perception,
                     imageproc::BinaryImage* auto_picture_mask,
                     imageproc::BinaryImage* speckles_image,
                     DebugImages* dbg,
                     const PageId& p_pageId,
                     const intrusive_ptr<Settings>& p_settings,
                     SplitImage* splitImage);

  QImage processWithoutDewarping(const TaskStatus& status,
                                 const FilterData& input,
                                 ZoneSet& picture_zones,
                                 const ZoneSet& fill_zones,
                                 imageproc::BinaryImage* auto_picture_mask,
                                 imageproc::BinaryImage* speckles_image,
                                 DebugImages* dbg,
                                 const PageId& p_pageId,
                                 const intrusive_ptr<Settings>& p_settings,
                                 SplitImage* splitImage);

  QImage processWithDewarping(const TaskStatus& status,
                              const FilterData& input,
                              ZoneSet& picture_zones,
                              const ZoneSet& fill_zones,
                              dewarping::DistortionModel& distortion_model,
                              const DepthPerception& depth_perception,
                              imageproc::BinaryImage* auto_picture_mask,
                              imageproc::BinaryImage* speckles_image,
                              DebugImages* dbg,
                              const PageId& p_pageId,
                              const intrusive_ptr<Settings>& p_settings,
                              SplitImage* splitImage);

  void movePointToTopMargin(BinaryImage& bw_image, XSpline& spline, int idx) const;

  void movePointToBottomMargin(BinaryImage& bw_image, XSpline& spline, int idx) const;

  void drawPoint(QImage& image, const QPointF& pt) const;

  void deskew(QImage* image, double angle, const QColor& outside_color) const;

  double maybe_deskew(QImage* p_dewarped, DewarpingOptions dewarping_options, const QColor& outside_color) const;

  void movePointToTopMargin(BinaryImage& bw_image, std::vector<QPointF>& polyline, int idx) const;

  void movePointToBottomMargin(BinaryImage& bw_image, std::vector<QPointF>& polyline, int idx) const;

  float vert_border_skew_angle(const QPointF& top, const QPointF& bottom) const;

  void setupTrivialDistortionModel(dewarping::DistortionModel& distortion_model) const;

  static dewarping::CylindricalSurfaceDewarper createDewarper(const dewarping::DistortionModel& distortion_model,
                                                              const QTransform& distortion_model_to_target,
                                                              double depth_perception);

  QImage dewarp(const QTransform& orig_to_src,
                const QImage& src,
                const QTransform& src_to_output,
                const dewarping::DistortionModel& distortion_model,
                const DepthPerception& depth_perception,
                const QColor& bg_color) const;

  static QSize from300dpi(const QSize& size, const Dpi& target_dpi);

  static QSize to300dpi(const QSize& size, const Dpi& source_dpi);

  static QImage convertToRGBorRGBA(const QImage& src);

  static void fillMarginsInPlace(QImage& image,
                                 const QPolygonF& content_poly,
                                 const QColor& color,
                                 bool antialiasing = true);

  static void fillMarginsInPlace(BinaryImage& image, const QPolygonF& content_poly, const BWColor& color);

  static void fillMarginsInPlace(QImage& image, const BinaryImage& content_mask, const QColor& color);

  static void fillMarginsInPlace(BinaryImage& image, const BinaryImage& content_mask, const BWColor& color);

  static imageproc::GrayImage normalizeIlluminationGray(const TaskStatus& status,
                                                        const QImage& input,
                                                        const QPolygonF& area_to_consider,
                                                        const QTransform& xform,
                                                        const QRect& target_rect,
                                                        imageproc::GrayImage* background = nullptr,
                                                        DebugImages* dbg = nullptr);

  imageproc::GrayImage detectPictures(const imageproc::GrayImage& input_300dpi,
                                      const TaskStatus& status,
                                      DebugImages* dbg = nullptr) const;

  imageproc::BinaryImage estimateBinarizationMask(const TaskStatus& status,
                                                  const imageproc::GrayImage& gray_source,
                                                  const QRect& source_rect,
                                                  const QRect& source_sub_rect,
                                                  DebugImages* dbg) const;

  void modifyBinarizationMask(imageproc::BinaryImage& bw_mask, const QRect& mask_rect, const ZoneSet& zones) const;

  imageproc::BinaryThreshold adjustThreshold(imageproc::BinaryThreshold threshold) const;

  imageproc::BinaryThreshold calcBinarizationThreshold(const QImage& image, const imageproc::BinaryImage& mask) const;

  imageproc::BinaryThreshold calcBinarizationThreshold(const QImage& image,
                                                       const QPolygonF& crop_area,
                                                       const imageproc::BinaryImage* mask = nullptr) const;

  BinaryImage binarize(const QImage& image) const;

  imageproc::BinaryImage binarize(const QImage& image, const imageproc::BinaryImage& mask) const;

  imageproc::BinaryImage binarize(const QImage& image,
                                  const QPolygonF& crop_area,
                                  const imageproc::BinaryImage* mask = nullptr) const;

  void maybeDespeckleInPlace(imageproc::BinaryImage& image,
                             const QRect& image_rect,
                             const QRect& mask_rect,
                             double level,
                             imageproc::BinaryImage* speckles_img,
                             const Dpi& dpi,
                             const TaskStatus& status,
                             DebugImages* dbg) const;

  static QImage smoothToGrayscale(const QImage& src, const Dpi& dpi);

  static void morphologicalSmoothInPlace(imageproc::BinaryImage& img, const TaskStatus& status);

  static void hitMissReplaceAllDirections(imageproc::BinaryImage& img,
                                          const char* pattern,
                                          int pattern_width,
                                          int pattern_height);

  static QSize calcLocalWindowSize(const Dpi& dpi);

  void applyFillZonesInPlace(QImage& img,
                             const ZoneSet& zones,
                             const boost::function<QPointF(const QPointF&)>& orig_to_output,
                             const QTransform& postTransform,
                             bool antialiasing = true) const;

  void applyFillZonesInPlace(QImage& img,
                             const ZoneSet& zones,
                             const boost::function<QPointF(const QPointF&)>& orig_to_output,
                             bool antialiasing = true) const;

  void applyFillZonesInPlace(QImage& img,
                             const ZoneSet& zones,
                             const QTransform& postTransform,
                             bool antialiasing = true) const;

  void applyFillZonesInPlace(QImage& img, const ZoneSet& zones, bool antialiasing = true) const;

  void applyFillZonesInPlace(imageproc::BinaryImage& img,
                             const ZoneSet& zones,
                             const boost::function<QPointF(const QPointF&)>& orig_to_output,
                             const QTransform& postTransform) const;

  void applyFillZonesInPlace(imageproc::BinaryImage& img,
                             const ZoneSet& zones,
                             const boost::function<QPointF(const QPointF&)>& orig_to_output) const;

  void applyFillZonesInPlace(imageproc::BinaryImage& img, const ZoneSet& zones, const QTransform& postTransform) const;

  void applyFillZonesInPlace(imageproc::BinaryImage& img, const ZoneSet& zones) const;

  void applyFillZonesToMixedInPlace(QImage& img,
                                    const ZoneSet& zones,
                                    const imageproc::BinaryImage& picture_mask,
                                    bool binary_mode) const;

  void applyFillZonesToMixedInPlace(QImage& img,
                                    const ZoneSet& zones,
                                    const boost::function<QPointF(const QPointF&)>& orig_to_output,
                                    const QTransform& postTransform,
                                    const imageproc::BinaryImage& picture_mask,
                                    bool binary_mode) const;

  QImage segmentImage(const BinaryImage& image, const QImage& color_image) const;

  QImage posterizeImage(const QImage& image, const QColor& background_color = Qt::white) const;

  Dpi m_dpi;
  ColorParams m_colorParams;
  SplittingOptions m_splittingOptions;
  PictureShapeOptions m_pictureShapeOptions;
  DewarpingOptions m_dewarpingOptions;
  OutputProcessingParams m_outputProcessingParams;

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

  double m_despeckleLevel;

  // store additional transformations after processing such as post deskew after dewarping
  QTransform postTransform;
};
}  // namespace output
#endif  // ifndef OUTPUT_OUTPUTGENERATOR_H_
