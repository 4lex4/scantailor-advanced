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

#include "imageproc/Connectivity.h"
#include "Dpi.h"
#include "ColorParams.h"
#include "Params.h"
#include "DepthPerception.h"
#include "DespeckleLevel.h"
#include "DewarpingOptions.h"
#include "ImageTransformation.h"
#include <boost/function.hpp>
#include <QSize>
#include <QRect>
#include <QTransform>
#include <QColor>
#include <QPointF>
#include <QLineF>
#include <QPolygonF>
#include <vector>
#include <utility>
#include <stdint.h>
#include "Params.h"
#include "PageId.h"
#include "intrusive_ptr.h"
#include "Settings.h"
#include <QMessageBox>
#include "TiffWriter.h"
#include <QtCore/qmath.h>
#include <QFile>
#include "imageproc/SkewFinder.h"
#include "SplitImage.h"

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
}

namespace dewarping {
    class DistortionModel;
    class CylindricalSurfaceDewarper;
}
using namespace imageproc;
namespace output {
    class OutputGenerator {
    public:
        OutputGenerator(Dpi const& dpi,
                        ColorParams const& color_params,
                        SplittingOptions splittingOptions,
                        DespeckleLevel despeckle_level,
                        ImageTransformation const& xform,
                        QPolygonF const& content_rect_phys);

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
        QImage process(TaskStatus const& status,
                       FilterData const& input,
                       ZoneSet& picture_zones,
                       ZoneSet const& fill_zones,
                       PictureShapeOptions picture_shape_options,
                       DewarpingOptions dewarping_options,
                       dewarping::DistortionModel& distortion_model,
                       DepthPerception const& depth_perception,
                       imageproc::BinaryImage* auto_picture_mask,
                       imageproc::BinaryImage* speckles_image,
                       DebugImages* dbg,
                       PageId const& p_pageId,
                       intrusive_ptr<Settings> const& p_settings,
                       SplitImage* splitImage);

        QSize outputImageSize() const;

        /**
         * \brief Returns the content rectangle in output image coordinates.
         */
        QRect outputContentRect() const;

        const QTransform& getPostTransform() const;

    private:
        QImage processImpl(TaskStatus const& status,
                           FilterData const& input,
                           ZoneSet& picture_zones,
                           ZoneSet const& fill_zones,
                           PictureShapeOptions picture_shape_options,
                           DewarpingOptions dewarping_options,
                           dewarping::DistortionModel& distortion_model,
                           DepthPerception const& depth_perception,
                           imageproc::BinaryImage* auto_picture_mask,
                           imageproc::BinaryImage* speckles_image,
                           DebugImages* dbg,
                           PageId const& p_pageId,
                           intrusive_ptr<Settings> const& p_settings,
                           SplitImage* splitImage);

        QImage processWithoutDewarping(TaskStatus const& status,
                                       FilterData const& input,
                                       ZoneSet& picture_zones,
                                       ZoneSet const& fill_zones,
                                       PictureShapeOptions picture_shape_options,
                                       imageproc::BinaryImage* auto_picture_mask,
                                       imageproc::BinaryImage* speckles_image,
                                       DebugImages* dbg,
                                       PageId const& p_pageId,
                                       intrusive_ptr<Settings> const& p_settings,
                                       SplitImage* splitImage);

        QImage processWithDewarping(TaskStatus const& status,
                                    FilterData const& input,
                                    ZoneSet& picture_zones,
                                    ZoneSet const& fill_zones,
                                    PictureShapeOptions picture_shape_options,
                                    DewarpingOptions dewarping_options,
                                    dewarping::DistortionModel& distortion_model,
                                    DepthPerception const& depth_perception,
                                    imageproc::BinaryImage* auto_picture_mask,
                                    imageproc::BinaryImage* speckles_image,
                                    DebugImages* dbg,
                                    PageId const& p_pageId,
                                    intrusive_ptr<Settings> const& p_settings,
                                    SplitImage* splitImage);

        void movePointToTopMargin(BinaryImage& bw_image, XSpline& spline, int idx) const;

        void movePointToBottomMargin(BinaryImage& bw_image, XSpline& spline, int idx) const;

        void drawPoint(QImage& image, QPointF const& pt) const;

        void deskew(QImage* image, double angle, const QColor& outside_color) const;

        double maybe_deskew(QImage* p_dewarped,
                            DewarpingOptions dewarping_options,
                            const QColor& outside_color) const;

        void movePointToTopMargin(BinaryImage& bw_image, std::vector<QPointF>& polyline, int idx) const;

        void movePointToBottomMargin(BinaryImage& bw_image, std::vector<QPointF>& polyline, int idx) const;

        float vert_border_skew_angle(QPointF const& top, QPointF const& bottom) const;

        void setupTrivialDistortionModel(dewarping::DistortionModel& distortion_model) const;

        static dewarping::CylindricalSurfaceDewarper createDewarper(dewarping::DistortionModel const& distortion_model,
                                                                    QTransform const& distortion_model_to_target,
                                                                    double depth_perception);

        QImage dewarp(QTransform const& orig_to_src,
                      QImage const& src,
                      QTransform const& src_to_output,
                      dewarping::DistortionModel const& distortion_model,
                      DepthPerception const& depth_perception,
                      QColor const& bg_color) const;

        static QSize from300dpi(QSize const& size, Dpi const& target_dpi);

        static QSize to300dpi(QSize const& size, Dpi const& source_dpi);

        static QImage convertToRGBorRGBA(QImage const& src);

        static void fillMarginsInPlace(QImage& image, QPolygonF const& content_poly, QColor const& color);

        static void fillMarginsInPlace(BinaryImage& image, QPolygonF const& content_poly, BWColor const& color);

        static void fillMarginsInPlace(QImage& image, BinaryImage const& content_mask, QColor const& color);

        static void fillMarginsInPlace(BinaryImage& image, BinaryImage const& content_mask, BWColor const& color);

        static imageproc::GrayImage normalizeIlluminationGray(TaskStatus const& status,
                                                              QImage const& input,
                                                              QPolygonF const& area_to_consider,
                                                              QTransform const& xform,
                                                              QRect const& target_rect,
                                                              imageproc::GrayImage* background = nullptr,
                                                              DebugImages* dbg = nullptr);

        static imageproc::GrayImage detectPictures(imageproc::GrayImage const& input_300dpi,
                                                   TaskStatus const& status,
                                                   DebugImages* dbg = nullptr);

        imageproc::BinaryImage estimateBinarizationMask(TaskStatus const& status,
                                                        imageproc::GrayImage const& gray_source,
                                                        QRect const& source_rect,
                                                        QRect const& source_sub_rect,
                                                        DebugImages* const dbg) const;

        void
        modifyBinarizationMask(imageproc::BinaryImage& bw_mask, QRect const& mask_rect, ZoneSet const& zones) const;

        imageproc::BinaryThreshold adjustThreshold(imageproc::BinaryThreshold threshold) const;

        imageproc::BinaryThreshold
        calcBinarizationThreshold(QImage const& image, imageproc::BinaryImage const& mask) const;

        imageproc::BinaryThreshold calcBinarizationThreshold(QImage const& image,
                                                             QPolygonF const& crop_area,
                                                             imageproc::BinaryImage const* mask = nullptr) const;

        BinaryImage binarize(QImage const& image) const;

        imageproc::BinaryImage binarize(QImage const& image, imageproc::BinaryImage const& mask) const;

        imageproc::BinaryImage binarize(QImage const& image,
                                        QPolygonF const& crop_area,
                                        imageproc::BinaryImage const* mask = nullptr) const;

        void maybeDespeckleInPlace(imageproc::BinaryImage& image,
                                   QRect const& image_rect,
                                   QRect const& mask_rect,
                                   DespeckleLevel level,
                                   imageproc::BinaryImage* speckles_img,
                                   Dpi const& dpi,
                                   TaskStatus const& status,
                                   DebugImages* dbg) const;

        static QImage smoothToGrayscale(QImage const& src, Dpi const& dpi);

        static void morphologicalSmoothInPlace(imageproc::BinaryImage& img, TaskStatus const& status);

        static void hitMissReplaceAllDirections(imageproc::BinaryImage& img,
                                                char const* pattern,
                                                int pattern_width,
                                                int pattern_height);

        static QSize calcLocalWindowSize(Dpi const& dpi);

        static QImage normalizeIllumination(QImage const& gray_input, DebugImages* dbg);

        QImage transformAndNormalizeIllumination(QImage const& gray_input,
                                                 DebugImages* dbg,
                                                 QImage const* morph_background = nullptr) const;

        QImage transformAndNormalizeIllumination2(QImage const& gray_input,
                                                  DebugImages* dbg,
                                                  QImage const* morph_background = nullptr) const;

        void applyFillZonesInPlace(QImage& img,
                                   ZoneSet const& zones,
                                   boost::function<QPointF(QPointF const&)> const& orig_to_output,
                                   QTransform const& postTransform) const;

        void applyFillZonesInPlace(QImage& img,
                                   ZoneSet const& zones,
                                   boost::function<QPointF(QPointF const&)> const& orig_to_output) const;

        void applyFillZonesInPlace(QImage& img, ZoneSet const& zones) const;

        void applyFillZonesInPlace(imageproc::BinaryImage& img,
                                   ZoneSet const& zones,
                                   boost::function<QPointF(QPointF const&)> const& orig_to_output,
                                   QTransform const& postTransform) const;

        void applyFillZonesInPlace(imageproc::BinaryImage& img,
                                   ZoneSet const& zones,
                                   boost::function<QPointF(QPointF const&)> const& orig_to_output) const;

        void applyFillZonesInPlace(imageproc::BinaryImage& img, ZoneSet const& zones) const;

        void applyFillZonesToMaskInPlace(imageproc::BinaryImage& mask,
                                         ZoneSet const& zones,
                                         boost::function<QPointF(QPointF const&)> const& orig_to_output,
                                         QTransform const& postTransform) const;

        void applyFillZonesToMaskInPlace(imageproc::BinaryImage& mask,
                                         ZoneSet const& zones,
                                         QTransform const& postTransform) const;

        void applyFillZonesToMaskInPlace(imageproc::BinaryImage& mask,
                                         ZoneSet const& zones,
                                         boost::function<QPointF(QPointF const&)> const& orig_to_output) const;

        void applyFillZonesToMaskInPlace(imageproc::BinaryImage& mask, ZoneSet const& zones) const;

        Dpi m_dpi;
        ColorParams m_colorParams;
        SplittingOptions m_splittingOptions;

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

        DespeckleLevel m_despeckleLevel;

        // store additional transformations after processing such as post deskew after dewarping
        QTransform postTransform;
    };
}  // namespace output
#endif  // ifndef OUTPUT_OUTPUTGENERATOR_H_
