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

#ifndef OUTPUT_OUTPUT_IMAGE_PARAMS_H_
#define OUTPUT_OUTPUT_IMAGE_PARAMS_H_

#include <ImageTransformation.h>
#include <QRect>
#include <QSize>
#include "ColorParams.h"
#include "DepthPerception.h"
#include "DespeckleLevel.h"
#include "DewarpingOptions.h"
#include "Dpi.h"
#include "OutputProcessingParams.h"
#include "Params.h"
#include "dewarping/DistortionModel.h"

class ImageTransformation;
class QDomDocument;
class QDomElement;
class QTransform;

namespace output {
/**
 * \brief Parameters of the output image used to determine if we need to re-generate it.
 */
class OutputImageParams {
 public:
  OutputImageParams(const QSize& out_image_size,
                    const QRect& content_rect,
                    ImageTransformation xform,
                    const Dpi& dpi,
                    const ColorParams& color_params,
                    const SplittingOptions& splittingOptions,
                    const DewarpingOptions& dewarping_options,
                    const dewarping::DistortionModel& distortion_model,
                    const DepthPerception& depth_perception,
                    double despeckle_level,
                    const PictureShapeOptions& picture_shape_options,
                    const OutputProcessingParams& output_processing_params,
                    bool is_black_on_white);

  explicit OutputImageParams(const QDomElement& el);

  const DewarpingOptions& dewarpingMode() const;

  const dewarping::DistortionModel& distortionModel() const;

  void setDistortionModel(const dewarping::DistortionModel& model);

  const DepthPerception& depthPerception() const;

  double despeckleLevel() const;

  void setOutputProcessingParams(const OutputProcessingParams& outputProcessingParams);

  const PictureShapeOptions& getPictureShapeOptions() const;

  const QPolygonF& getCropArea() const;

  void setBlackOnWhite(bool blackOnWhite);

  QDomElement toXml(QDomDocument& doc, const QString& name) const;

  /**
   * \brief Returns true if two sets of parameters are close enough
   *        to avoid re-generating the output image.
   */
  bool matches(const OutputImageParams& other) const;

 private:
  class PartialXform {
   public:
    PartialXform();

    PartialXform(const QTransform& xform);

    explicit PartialXform(const QDomElement& el);

    QDomElement toXml(QDomDocument& doc, const QString& name) const;

    bool matches(const PartialXform& other) const;

   private:
    static bool closeEnough(double v1, double v2);

    double m_11;
    double m_12;
    double m_21;
    double m_22;
  };

  static bool colorParamsMatch(const ColorParams& cp1,
                               double dl1,
                               const SplittingOptions& so1,
                               const ColorParams& cp2,
                               double dl2,
                               const SplittingOptions& so2);

  /** Pixel size of the output image. */
  QSize m_size;

  /** Content rectangle in output image coordinates. */
  QRect m_contentRect;

  /** Crop area in output image coordinates. */
  QPolygonF m_cropArea;

  /**
   * Some parameters from the transformation matrix that maps
   * source image coordinates to unscaled (disregarding dpi changes)
   * output image coordinates.
   */
  PartialXform m_partialXform;

  /** DPI of the output image. */
  Dpi m_dpi;

  /** Non-geometric parameters used to generate the output image. */
  ColorParams m_colorParams;

  /** Parameters used to generate the split output images. */
  SplittingOptions m_splittingOptions;

  /** Shape of the pictures in image */
  PictureShapeOptions m_pictureShapeOptions;

  /** Two curves and two lines connecting their endpoints.  Used for dewarping. */
  dewarping::DistortionModel m_distortionModel;

  /** \see imageproc::CylindricalSurfaceDewarper */
  DepthPerception m_depthPerception;

  /** Dewarping mode (Off / Auto / Manual) and options */
  DewarpingOptions m_dewarpingOptions;

  /** Despeckle level of the output image. */
  double m_despeckleLevel;

  /** Per-page params set while processing. */
  OutputProcessingParams m_outputProcessingParams;

  /** Whether the page has dark content on light background */
  bool m_blackOnWhite;
};
}  // namespace output
#endif  // ifndef OUTPUT_OUTPUT_IMAGE_PARAMS_H_
