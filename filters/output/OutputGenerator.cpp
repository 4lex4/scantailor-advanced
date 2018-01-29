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

#include "OutputGenerator.h"
#include "FilterData.h"
#include "TaskStatus.h"
#include "Utils.h"
#include "DebugImages.h"
#include "EstimateBackground.h"
#include "RenderParams.h"
#include "Dpm.h"
#include "FillColorProperty.h"
#include "dewarping/CylindricalSurfaceDewarper.h"
#include "dewarping/TextLineTracer.h"
#include "dewarping/TopBottomEdgeTracer.h"
#include "dewarping/DistortionModelBuilder.h"
#include "dewarping/DewarpingPointMapper.h"
#include "dewarping/RasterDewarper.h"
#include "imageproc/Binarize.h"
#include "imageproc/Transform.h"
#include "imageproc/Scale.h"
#include "imageproc/Morphology.h"
#include "imageproc/ConnCompEraser.h"
#include "imageproc/SeedFill.h"
#include "imageproc/Constants.h"
#include "imageproc/Grayscale.h"
#include "imageproc/RasterOp.h"
#include "imageproc/GrayRasterOp.h"
#include "imageproc/PolynomialSurface.h"
#include "imageproc/SavGolFilter.h"
#include "imageproc/DrawOver.h"
#include "imageproc/AdjustBrightness.h"
#include "imageproc/PolygonRasterizer.h"
#include "imageproc/ConnectivityMap.h"
#include "imageproc/InfluenceMap.h"
#include <boost/bind.hpp>
#include <QPainter>
#include <QDebug>
#include <imageproc/BackgroundColorCalculator.h>
#include <Despeckle.h>
#include "imageproc/OrthogonalRotation.h"

using namespace imageproc;
using namespace dewarping;

namespace output {
    namespace {
        struct RaiseAboveBackground {
            static uint8_t transform(uint8_t src, uint8_t dst) {
                // src: orig
                // dst: background (dst >= src)
                if (dst - src < 1) {
                    return 0xff;
                }
                const unsigned orig = src;
                const unsigned background = dst;

                return static_cast<uint8_t>((orig * 255 + background / 2) / background);
            }
        };

        struct CombineInverted {
            static uint8_t transform(uint8_t src, uint8_t dst) {
                const unsigned dilated = dst;
                const unsigned eroded = src;
                const unsigned res = 255 - (255 - dilated) * eroded / 255;

                return static_cast<uint8_t>(res);
            }
        };

/**
 * In picture areas we make sure we don't use pure black and pure white colors.
 * These are reserved for text areas.  This behaviour makes it possible to
 * detect those picture areas later and treat them differently, for example
 * encoding them as a background layer in DjVu format.
 */
        template<typename PixelType>
        PixelType reserveBlackAndWhite(PixelType color);

        template<>
        uint32_t reserveBlackAndWhite(uint32_t color) {
            // We handle both RGB32 and ARGB32 here.
            switch (color & 0x00FFFFFF) {
                case 0x00000000:
                    return 0xFF010101;
                case 0x00FFFFFF:
                    return 0xFFFEFEFE;
                default:
                    return color;
            }
        }

        template<>
        uint8_t reserveBlackAndWhite(uint8_t color) {
            switch (color) {
                case 0x00:
                    return 0x01;
                case 0xFF:
                    return 0xFE;
                default:
                    return color;
            }
        }

        template<typename PixelType>
        void reserveBlackAndWhite(QSize size, int stride, PixelType* data) {
            const int width = size.width();
            const int height = size.height();

            PixelType* line = data;
            for (int y = 0; y < height; ++y, line += stride) {
                for (int x = 0; x < width; ++x) {
                    line[x] = reserveBlackAndWhite<PixelType>(line[x]);
                }
            }
        }

        void reserveBlackAndWhite(QImage& img) {
            assert(img.depth() == 8 || img.depth() == 24 || img.depth() == 32);
            switch (img.format()) {
                case QImage::Format_Indexed8:
                    reserveBlackAndWhite(img.size(), img.bytesPerLine(), img.bits());
                    break;
                case QImage::Format_RGB32:
                case QImage::Format_ARGB32:
                    reserveBlackAndWhite(img.size(), img.bytesPerLine() / 4, (uint32_t*) img.bits());
                    break;
                default:;
            }
        }

        /**
         * Fills areas of \p mixed with pixels from \p bw_content in
         * areas where \p bw_mask is black.  Supported \p mixed image formats
         * are Indexed8 grayscale, RGB32 and ARGB32.
         * The \p MixedPixel type is uint8_t for Indexed8 grayscale and uint32_t
         * for RGB32 and ARGB32.
         */
        template<typename MixedPixel>
        void combineMixed(QImage& mixed,
                          const BinaryImage& bw_content,
                          const BinaryImage& bw_mask,
                          bool needReserveBlackAndWhite = true) {
            auto* mixed_line = reinterpret_cast<MixedPixel*>(mixed.bits());
            const int mixed_stride = mixed.bytesPerLine() / sizeof(MixedPixel);
            const uint32_t* bw_content_line = bw_content.data();
            const int bw_content_stride = bw_content.wordsPerLine();
            const uint32_t* bw_mask_line = bw_mask.data();
            const int bw_mask_stride = bw_mask.wordsPerLine();
            const int width = mixed.width();
            const int height = mixed.height();
            const uint32_t msb = uint32_t(1) << 31;

            for (int y = 0; y < height; ++y) {
                for (int x = 0; x < width; ++x) {
                    if (bw_mask_line[x >> 5] & (msb >> (x & 31))) {
                        // B/W content.

                        uint32_t tmp = bw_content_line[x >> 5];
                        tmp >>= (31 - (x & 31));
                        tmp &= uint32_t(1);
                        // Now it's 0 for white and 1 for black.
                        --tmp;  // 0 becomes 0xffffffff and 1 becomes 0.
                        tmp |= 0xff000000;  // Force opacity.
                        mixed_line[x] = static_cast<MixedPixel>(tmp);
                    } else {
                        // Non-B/W content.
                        if (needReserveBlackAndWhite) {
                            mixed_line[x] = reserveBlackAndWhite<MixedPixel>(mixed_line[x]);
                        }
                    }
                }
                mixed_line += mixed_stride;
                bw_content_line += bw_content_stride;
                bw_mask_line += bw_mask_stride;
            }
        }  // combineMixed

        void combineMixed(QImage& mixed,
                          const BinaryImage& bw_content,
                          const BinaryImage& bw_mask,
                          bool needReserveBlackAndWhite = true) {
            if ((mixed.format() != QImage::Format_Indexed8)
                && (mixed.format() != QImage::Format_RGB32)
                && (mixed.format() != QImage::Format_ARGB32)) {
                throw std::invalid_argument("Error: wrong image format.");
            }

            if (mixed.format() == QImage::Format_Indexed8) {
                combineMixed<uint8_t>(mixed, bw_content, bw_mask, needReserveBlackAndWhite);
            } else {
                combineMixed<uint32_t>(mixed, bw_content, bw_mask, needReserveBlackAndWhite);
            }
        }

        template<typename MixedPixel>
        void fillExcept(QImage& image, const BinaryImage& bw_mask, const QColor& color) {
            auto* image_line = reinterpret_cast<MixedPixel*>(image.bits());
            const int image_stride = image.bytesPerLine() / sizeof(MixedPixel);
            const uint32_t* bw_mask_line = bw_mask.data();
            const int bw_mask_stride = bw_mask.wordsPerLine();
            const int width = image.width();
            const int height = image.height();
            const uint32_t msb = uint32_t(1) << 31;
            const auto fillingPixel = static_cast<MixedPixel>(color.rgba());

            for (int y = 0; y < height; ++y) {
                for (int x = 0; x < width; ++x) {
                    if (!(bw_mask_line[x >> 5] & (msb >> (x & 31)))) {
                        image_line[x] = fillingPixel;
                    }
                }
                image_line += image_stride;
                bw_mask_line += bw_mask_stride;
            }
        }

        void fillExcept(BinaryImage& image, const BinaryImage& bw_mask, const BWColor color) {
            uint32_t* image_line = image.data();
            const int image_stride = image.wordsPerLine();
            const uint32_t* bw_mask_line = bw_mask.data();
            const int bw_mask_stride = bw_mask.wordsPerLine();
            const int width = image.width();
            const int height = image.height();
            const uint32_t msb = uint32_t(1) << 31;

            for (int y = 0; y < height; ++y) {
                for (int x = 0; x < width; ++x) {
                    if (!(bw_mask_line[x >> 5] & (msb >> (x & 31)))) {
                        if (color == BLACK) {
                            image_line[x >> 5] |= (msb >> (x & 31));
                        } else {
                            image_line[x >> 5] &= ~(msb >> (x & 31));
                        }
                    }
                }
                image_line += image_stride;
                bw_mask_line += bw_mask_stride;
            }
        }

        template<typename MixedPixel>
        void applyMask(QImage& image, const BinaryImage& bw_mask, const BWColor filling_color = WHITE) {
            auto* image_line = reinterpret_cast<MixedPixel*>(image.bits());
            const int image_stride = image.bytesPerLine() / sizeof(MixedPixel);
            const uint32_t* bw_mask_line = bw_mask.data();
            const int bw_mask_stride = bw_mask.wordsPerLine();
            const int width = image.width();
            const int height = image.height();
            const uint32_t msb = uint32_t(1) << 31;
            const auto fillingPixel = static_cast<MixedPixel>(
                    (filling_color == WHITE) ? 0xffffffff : 0x00000000
            );

            for (int y = 0; y < height; ++y) {
                for (int x = 0; x < width; ++x) {
                    if (!(bw_mask_line[x >> 5] & (msb >> (x & 31)))) {
                        image_line[x] = fillingPixel;
                    }
                }
                image_line += image_stride;
                bw_mask_line += bw_mask_stride;
            }
        }

        void applyMask(QImage& image, const BinaryImage& bw_mask, const BWColor filling_color = WHITE) {
            if ((image.format() != QImage::Format_Indexed8)
                && (image.format() != QImage::Format_RGB32)
                && (image.format() != QImage::Format_ARGB32)) {
                throw std::invalid_argument("Error: wrong image format.");
            }

            if (image.format() == QImage::Format_Indexed8) {
                applyMask<uint8_t>(image, bw_mask, filling_color);
            } else {
                applyMask<uint32_t>(image, bw_mask, filling_color);
            }
        }

        void removeAutoPictureZones(ZoneSet& picture_zones) {
            for (auto it = picture_zones.begin(); it != picture_zones.end();) {
                const Zone& zone = *it;
                if (zone.properties().locateOrDefault<ZoneCategoryProperty>()->zone_category()
                    == ZoneCategoryProperty::RECTANGULAR_OUTLINE) {
                    it = picture_zones.erase(it);
                } else {
                    ++it;
                }
            }
        }
    }      // namespace

    OutputGenerator::OutputGenerator(const Dpi& dpi,
                                     const ColorParams& color_params,
                                     const SplittingOptions& splitting_options,
                                     const PictureShapeOptions& picture_shape_options,
                                     const DewarpingOptions& dewarping_options,
                                     const OutputProcessingParams& output_processing_params,
                                     DespeckleLevel despeckle_level,
                                     const ImageTransformation& xform,
                                     const QPolygonF& content_rect_phys)
            : m_dpi(dpi),
              m_colorParams(color_params),
              m_splittingOptions(splitting_options),
              m_pictureShapeOptions(picture_shape_options),
              m_dewarpingOptions(dewarping_options),
              m_outputProcessingParams(output_processing_params),
              m_xform(xform),
              m_outRect(xform.resultingRect().toRect()),
              m_contentRect(xform.transform().map(content_rect_phys).boundingRect().toRect()),
              m_despeckleLevel(despeckle_level) {
        assert(m_outRect.topLeft() == QPoint(0, 0));

        if (!m_contentRect.isEmpty()) {
            // prevents a crash due to round error on transforming virtual coordinates to output image coordinates
            // when m_contentRect coordinates could exceed m_outRect ones by 1 px
            m_contentRect = m_contentRect.intersected(m_outRect);
            // Note that QRect::contains(<empty rect>) always returns false, so we don't use it here.
            assert(m_outRect.contains(m_contentRect.topLeft()) && m_outRect.contains(m_contentRect.bottomRight()));
        }
    }

    QImage OutputGenerator::process(const TaskStatus& status,
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
                                    SplitImage* splitImage) {
        QImage image(
                processImpl(
                        status, input, picture_zones, fill_zones,
                        distortion_model, depth_perception,
                        auto_picture_mask, speckles_image, dbg, p_pageId,
                        p_settings,
                        splitImage
                )
        );
        // Set the correct DPI.
        const RenderParams renderParams(m_colorParams, m_splittingOptions);
        const Dpm output_dpm(m_dpi);

        if (!renderParams.splitOutput()) {
            assert(!image.isNull());

            image.setDotsPerMeterX(output_dpm.horizontal());
            image.setDotsPerMeterY(output_dpm.vertical());
        } else {
            splitImage->applyToLayerImages([&output_dpm](QImage& img) {
                img.setDotsPerMeterX(output_dpm.horizontal());
                img.setDotsPerMeterY(output_dpm.vertical());
            });
        }

        return image;
    }

    QSize OutputGenerator::outputImageSize() const {
        return m_outRect.size();
    }

    QRect OutputGenerator::outputContentRect() const {
        return m_contentRect;
    }

    GrayImage OutputGenerator::normalizeIlluminationGray(const TaskStatus& status,
                                                         const QImage& input,
                                                         const QPolygonF& area_to_consider,
                                                         const QTransform& xform,
                                                         const QRect& target_rect,
                                                         GrayImage* background,
                                                         DebugImages* const dbg) {
        GrayImage to_be_normalized(
                transformToGray(
                        input, xform, target_rect, OutsidePixels::assumeWeakNearest()
                )
        );
        if (dbg) {
            dbg->add(to_be_normalized, "to_be_normalized");
        }

        status.throwIfCancelled();

        QPolygonF transformed_consideration_area(xform.map(area_to_consider));
        transformed_consideration_area.translate(-target_rect.topLeft());

        const PolynomialSurface bg_ps(
                estimateBackground(
                        to_be_normalized, transformed_consideration_area,
                        status, dbg
                )
        );

        status.throwIfCancelled();

        GrayImage bg_img(bg_ps.render(to_be_normalized.size()));
        if (dbg) {
            dbg->add(bg_img, "background");
        }
        if (background) {
            *background = bg_img;
        }

        status.throwIfCancelled();

        grayRasterOp<RaiseAboveBackground>(bg_img, to_be_normalized);
        if (dbg) {
            dbg->add(bg_img, "normalized_illumination");
        }

        return bg_img;
    }  // OutputGenerator::normalizeIlluminationGray

    imageproc::BinaryImage OutputGenerator::estimateBinarizationMask(const TaskStatus& status,
                                                                     const GrayImage& gray_source,
                                                                     const QRect& source_rect,
                                                                     const QRect& source_sub_rect,
                                                                     DebugImages* const dbg) const {
        assert(source_rect.contains(source_sub_rect));

        // If we need to strip some of the margins from a grayscale
        // image, we may actually do it without copying anything.
        // We are going to construct a QImage from existing data.
        // That image won't own that data, but gray_source is not
        // going anywhere, so it's fine.

        GrayImage trimmed_image;

        if (source_rect == source_sub_rect) {
            trimmed_image = gray_source;  // Shallow copy.
        } else {
            // Sub-rectangle in input image coordinates.
            QRect relative_subrect(source_sub_rect);
            relative_subrect.moveTopLeft(
                    source_sub_rect.topLeft() - source_rect.topLeft()
            );

            const int stride = gray_source.stride();
            const int offset = relative_subrect.top() * stride
                               + relative_subrect.left();

            trimmed_image = GrayImage(QImage(
                    gray_source.data() + offset,
                    relative_subrect.width(), relative_subrect.height(),
                    stride, QImage::Format_Indexed8
            ));
        }

        status.throwIfCancelled();

        const QSize downscaled_size(to300dpi(trimmed_image.size(), m_dpi));

        // A 300dpi version of trimmed_image.
        GrayImage downscaled_input(
                scaleToGray(trimmed_image, downscaled_size)
        );
        trimmed_image = GrayImage();  // Save memory.
        status.throwIfCancelled();

        // Light areas indicate pictures.
        GrayImage picture_areas(detectPictures(downscaled_input, status, dbg));
        downscaled_input = GrayImage();  // Save memory.
        status.throwIfCancelled();

        const BinaryThreshold threshold(48);
        // Scale back to original size.
        picture_areas = scaleToGray(
                picture_areas, source_sub_rect.size()
        );

        return BinaryImage(picture_areas, threshold);
    }  // OutputGenerator::estimateBinarizationMask

    void OutputGenerator::modifyBinarizationMask(imageproc::BinaryImage& bw_mask,
                                                 const QRect& mask_rect,
                                                 const ZoneSet& zones) const {
        QTransform xform(m_xform.transform());
        xform *= QTransform().translate(-mask_rect.x(), -mask_rect.y());

        typedef PictureLayerProperty PLP;

        // Pass 1: ERASER1
        for (const Zone& zone : zones) {
            if (zone.properties().locateOrDefault<PLP>()->layer() == PLP::ERASER1) {
                const QPolygonF poly(zone.spline().toPolygon());
                PolygonRasterizer::fill(bw_mask, BLACK, xform.map(poly), Qt::WindingFill);
            }
        }

        // Pass 2: PAINTER2
        for (const Zone& zone : zones) {
            if (zone.properties().locateOrDefault<PLP>()->layer() == PLP::PAINTER2) {
                const QPolygonF poly(zone.spline().toPolygon());
                PolygonRasterizer::fill(bw_mask, WHITE, xform.map(poly), Qt::WindingFill);
            }
        }

        // Pass 1: ERASER3
        for (const Zone& zone : zones) {
            if (zone.properties().locateOrDefault<PLP>()->layer() == PLP::ERASER3) {
                const QPolygonF poly(zone.spline().toPolygon());
                PolygonRasterizer::fill(bw_mask, BLACK, xform.map(poly), Qt::WindingFill);
            }
        }
    }

    QImage OutputGenerator::processImpl(const TaskStatus& status,
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
                                        SplitImage* splitImage) {
        if ((m_dewarpingOptions.dewarpingMode() == AUTO)
            || (m_dewarpingOptions.dewarpingMode() == MARGINAL)
            || ((m_dewarpingOptions.dewarpingMode() == MANUAL) && distortion_model.isValid())) {
            return processWithDewarping(
                    status, input, picture_zones, fill_zones,
                    distortion_model, depth_perception,
                    auto_picture_mask, speckles_image, dbg, p_pageId,
                    p_settings,
                    splitImage
            );
        } else {
            return processWithoutDewarping(
                    status, input, picture_zones, fill_zones,
                    auto_picture_mask, speckles_image, dbg, p_pageId,
                    p_settings,
                    splitImage
            );
        }
    }

    QImage OutputGenerator::processWithoutDewarping(const TaskStatus& status,
                                                    const FilterData& input,
                                                    ZoneSet& picture_zones,
                                                    const ZoneSet& fill_zones,
                                                    imageproc::BinaryImage* auto_picture_mask,
                                                    imageproc::BinaryImage* speckles_image,
                                                    DebugImages* dbg,
                                                    const PageId& p_pageId,
                                                    const intrusive_ptr<Settings>& p_settings,
                                                    SplitImage* splitImage) {
        const RenderParams render_params(m_colorParams, m_splittingOptions);

        const QSize target_size(m_outRect.size().expandedTo(QSize(1, 1)));
        if (m_contentRect.isEmpty()) {
            QImage emptyImage(BinaryImage(target_size, WHITE).toQImage());
            if (!render_params.splitOutput()) {
                return emptyImage;
            } else {
                splitImage->setForegroundImage(emptyImage);
                splitImage->setBackgroundImage(emptyImage.convertToFormat(QImage::Format_Indexed8));
                return QImage();
            }
        }

        QRect contentRect = m_xform.resultingPreCropArea().boundingRect().toRect().intersected(
                render_params.cutMargins() ? m_contentRect : m_outRect);

        // The whole image minus the part cut off by the split line.
        const QRect big_margins_rect(
                m_xform.resultingPreCropArea().boundingRect().toRect()
        );

        // For various reasons, we need some whitespace around the content
        // area.  This is the number of pixels of such whitespace.
        const int content_margin = m_dpi.vertical() * 20 / 300;

        // The content area (in output image coordinates) extended
        // with content_margin.  Note that we prevent that extension
        // from reaching the neighboring page.
        const QRect small_margins_rect(
                contentRect.adjusted(
                        -content_margin, -content_margin,
                        content_margin, content_margin
                ).intersected(big_margins_rect)
        );

        // This is the area we are going to pass to estimateBackground().
        // estimateBackground() needs some margins around content, and
        // generally smaller margins are better, except when there is
        // some garbage that connects the content to the edge of the
        // image area.
        const QRect normalize_illumination_rect(
                small_margins_rect
        );

        QImage maybe_normalized;

        // Crop area in original image coordinates.
        const QPolygonF orig_image_crop_area(
                m_xform.transformBack().map(
                        m_xform.resultingPreCropArea()
                )
        );

        // Crop area in maybe_normalized image coordinates.
        QPolygonF normalize_illumination_crop_area(m_xform.resultingPreCropArea());
        normalize_illumination_crop_area.translate(-normalize_illumination_rect.topLeft());

        QColor outsideBackgroundColor;
        QColor outsideBackgroundColorBW;

        const bool needNormalizeIllumination
                = (render_params.normalizeIllumination() && render_params.needBinarization())
                  || (render_params.normalizeIlluminationColor() && !render_params.needBinarization());

        if (needNormalizeIllumination) {
            maybe_normalized = normalizeIlluminationGray(
                    status, input.grayImage(), orig_image_crop_area,
                    m_xform.transform(), normalize_illumination_rect, nullptr, dbg
            );
        } else {
            if (input.origImage().allGray()) {
                maybe_normalized = transformToGray(
                        input.grayImage(), m_xform.transform(),
                        normalize_illumination_rect, OutsidePixels::assumeColor(Qt::white)
                );
            } else {
                maybe_normalized = transform(
                        input.origImage(), m_xform.transform(),
                        normalize_illumination_rect, OutsidePixels::assumeColor(Qt::white)
                );
            }
        }

        if (needNormalizeIllumination && !input.origImage().allGray()) {
            assert(maybe_normalized.format() == QImage::Format_Indexed8);
            QImage tmp(
                    transform(
                            input.origImage(), m_xform.transform(),
                            normalize_illumination_rect,
                            OutsidePixels::assumeColor(outsideBackgroundColor)
                    )
            );

            status.throwIfCancelled();

            adjustBrightnessGrayscale(tmp, maybe_normalized);
            maybe_normalized = tmp;
            if (dbg) {
                dbg->add(maybe_normalized, "norm_illum_color");
            }
        }

        outsideBackgroundColor = BackgroundColorCalculator::calcDominantBackgroundColor(maybe_normalized);
        if (render_params.needBinarization()) {
            outsideBackgroundColorBW
                    = BackgroundColorCalculator::calcDominantBackgroundColorBW(maybe_normalized);

            if (!m_outputProcessingParams.isWhiteOnBlackAutoDetected()) {
                m_outputProcessingParams.setWhiteOnBlackMode(outsideBackgroundColorBW == Qt::black);

                m_outputProcessingParams.setWhiteOnBlackAutoDetected(true);
                p_settings->setOutputProcessingParams(p_pageId, m_outputProcessingParams);
            }
        }

        // Transform from the output image coordinates to the transformed image coordinates.
        QTransform output_to_transformed;
        output_to_transformed.translate(-normalize_illumination_rect.left(),
                                        -normalize_illumination_rect.top());

        fillMarginsInPlace(maybe_normalized, normalize_illumination_crop_area, outsideBackgroundColor);

        status.throwIfCancelled();

        if (render_params.binaryOutput()) {
            BinaryImage dst(target_size, WHITE);

            QImage maybe_smoothed;
            // We only do smoothing if we are going to do binarization later.
            if (!render_params.needSavitzkyGolaySmoothing()) {
                maybe_smoothed = maybe_normalized;
            } else {
                maybe_smoothed = smoothToGrayscale(maybe_normalized, m_dpi);
                if (dbg) {
                    dbg->add(maybe_smoothed, "smoothed");
                }
            }
            maybe_normalized = QImage();

            status.throwIfCancelled();

            BinaryImage bw_content(
                    binarize(maybe_smoothed, normalize_illumination_crop_area)
            );

            if (m_colorParams.colorCommonOptions().getFillingColor() == ColorCommonOptions::BACKGROUND) {
                BWColor fillColor = (outsideBackgroundColorBW == Qt::black) ? BLACK : WHITE;
                fillMarginsInPlace(bw_content, normalize_illumination_crop_area, fillColor);
                dst.fill(fillColor);
            }

            maybe_smoothed = QImage();
            if (dbg) {
                dbg->add(bw_content, "binarized_and_cropped");
            }

            status.throwIfCancelled();

            if (render_params.needMorphologicalSmoothing()) {
                morphologicalSmoothInPlace(bw_content, status);
                if (dbg) {
                    dbg->add(bw_content, "edges_smoothed");
                }
            }

            status.throwIfCancelled();

            const QRect src_rect(contentRect.translated(-normalize_illumination_rect.topLeft()));
            const QRect dst_rect(contentRect);
            rasterOp<RopSrc>(dst, dst_rect, bw_content, src_rect.topLeft());
            bw_content.release();  // Save memory.

            // It's important to keep despeckling the very last operation
            // affecting the binary part of the output. That's because
            // we will be reconstructing the input to this despeckling
            // operation from the final output file.
            maybeDespeckleInPlace(
                    dst, m_outRect, m_outRect, m_despeckleLevel,
                    speckles_image, m_dpi, status, dbg
            );

            applyFillZonesInPlace(dst, fill_zones);

            return dst.toQImage();
        }

        BinaryImage picture_mask_output;
        if (render_params.mixedOutput()) {
            BinaryImage bw_mask(small_margins_rect.size(), BLACK);

            if ((m_pictureShapeOptions.getPictureShape() != RECTANGULAR_SHAPE)
                || !m_outputProcessingParams.isAutoZonesFound()) {
                if (m_pictureShapeOptions.getPictureShape() != OFF_SHAPE) {
                    bw_mask = estimateBinarizationMask(
                            status, GrayImage(maybe_normalized),
                            normalize_illumination_rect,
                            small_margins_rect, dbg
                    );
                }

                removeAutoPictureZones(picture_zones);
                p_settings->setPictureZones(p_pageId, picture_zones);
                m_outputProcessingParams.setAutoZonesFound(false);
                p_settings->setOutputProcessingParams(p_pageId, m_outputProcessingParams);
            }
            if ((m_pictureShapeOptions.getPictureShape() == RECTANGULAR_SHAPE)
                && !m_outputProcessingParams.isAutoZonesFound()) {
                std::vector<QRect> areas;
                bw_mask.rectangularizeAreas(areas, WHITE, m_pictureShapeOptions.getSensitivity());

                QTransform xform1(m_xform.transform());
                xform1 *= output_to_transformed;
                QTransform inv_xform(xform1.inverted());

                for (auto i : areas) {
                    QRectF area0(i);
                    QPolygonF area1(area0);
                    QPolygonF area(inv_xform.map(area1));

                    Zone zone1(area);

                    picture_zones.add(zone1);
                }
                p_settings->setPictureZones(p_pageId, picture_zones);
                m_outputProcessingParams.setAutoZonesFound(true);
                p_settings->setOutputProcessingParams(p_pageId, m_outputProcessingParams);

                bw_mask.fill(BLACK);
            }

            if (dbg) {
                dbg->add(bw_mask, "bw_mask");
            }

            if (auto_picture_mask) {
                if (auto_picture_mask->size() != target_size) {
                    BinaryImage(target_size).swap(*auto_picture_mask);
                }
                auto_picture_mask->fill(BLACK);

                const QRect src_rect(contentRect.translated(-small_margins_rect.topLeft()));
                const QRect dst_rect(contentRect);
                rasterOp<RopSrc>(*auto_picture_mask, dst_rect, bw_mask, src_rect.topLeft());
            }

            status.throwIfCancelled();

            modifyBinarizationMask(bw_mask, small_margins_rect, picture_zones);
            if (dbg) {
                dbg->add(bw_mask, "bw_mask with zones");
            }

            {
                picture_mask_output = BinaryImage(target_size, BLACK);
                const QRect src_rect(contentRect.translated(-normalize_illumination_rect.topLeft()));
                const QRect dst_rect(contentRect);
                rasterOp<RopSrc>(picture_mask_output, dst_rect, bw_mask, src_rect.topLeft());
            }

            status.throwIfCancelled();

            if (render_params.needBinarization()) {
                QImage maybe_smoothed;
                if (!render_params.needSavitzkyGolaySmoothing()) {
                    maybe_smoothed = maybe_normalized;
                } else {
                    maybe_smoothed = smoothToGrayscale(maybe_normalized, m_dpi);
                    if (dbg) {
                        dbg->add(maybe_smoothed, "smoothed");
                    }
                }

                BinaryImage bw_mask_filled(bw_mask);
                fillMarginsInPlace(bw_mask_filled, normalize_illumination_crop_area, WHITE);

                BinaryImage bw_content(
                        binarize(maybe_smoothed, bw_mask_filled)
                );

                bw_mask.release();
                maybe_smoothed = QImage();  // Save memory.
                if (dbg) {
                    dbg->add(bw_content, "binarized_and_cropped");
                }

                status.throwIfCancelled();

                if (render_params.needMorphologicalSmoothing()) {
                    morphologicalSmoothInPlace(bw_content, status);
                    if (dbg) {
                        dbg->add(bw_content, "edges_smoothed");
                    }
                }

                // We don't want speckles in non-B/W areas, as they would
                // then get visualized on the Despeckling tab.
                status.throwIfCancelled();

                // It's important to keep despeckling the very last operation
                // affecting the binary part of the output. That's because
                // we will be reconstructing the input to this despeckling
                // operation from the final output file.
                maybeDespeckleInPlace(
                        bw_content, small_margins_rect, contentRect,
                        m_despeckleLevel, speckles_image, m_dpi, status, dbg
                );

                status.throwIfCancelled();

                if (needNormalizeIllumination && !render_params.normalizeIlluminationColor()) {
                    if (input.origImage().allGray()) {
                        maybe_normalized = transformToGray(
                                input.grayImage(), m_xform.transform(),
                                normalize_illumination_rect, OutsidePixels::assumeColor(outsideBackgroundColor)
                        );
                    } else {
                        maybe_normalized = transform(
                                input.origImage(), m_xform.transform(),
                                normalize_illumination_rect, OutsidePixels::assumeColor(outsideBackgroundColor)
                        );
                    }

                    status.throwIfCancelled();
                }

                if (render_params.originalBackground()) {
                    QImage original_background(maybe_normalized);
                    // reserving original black and white colors
                    reserveBlackAndWhite(original_background);
                    // subtracting bw content filling the content areas with white,
                    // considering the colors of the content and background
                    BinaryImage bw_content_mask(bw_content);
                    // we applying BW filling zones to the mask to create an ability to control
                    // the content of original background manually
                    applyFillZonesInPlace(bw_content_mask, fill_zones, output_to_transformed);
                    if (outsideBackgroundColorBW == Qt::white) {
                        bw_content_mask.invert();
                    }
                    applyMask(original_background,
                              bw_content_mask,
                              WHITE);
                    // applying picture mask filling the picture areas with black
                    applyMask(original_background, bw_mask_filled, BLACK);

                    status.throwIfCancelled();

                    QImage original_background_dst(target_size, original_background.format());
                    if (original_background.format() == QImage::Format_Indexed8) {
                        original_background_dst.setColorTable(createGrayscalePalette());
                    }
                    if (original_background_dst.isNull()) {
                        // Both the constructor and setColorTable() above can leave the image null.
                        throw std::bad_alloc();
                    }

                    QColor outsideOriginalBackgroundColor = outsideBackgroundColor;
                    if (m_colorParams.colorCommonOptions().getFillingColor() == ColorCommonOptions::WHITE) {
                        outsideOriginalBackgroundColor = Qt::white;
                    }
                    // reserving outside background color if it's black or white
                    if (outsideOriginalBackgroundColor == Qt::white) {
                        outsideOriginalBackgroundColor = QColor(254, 254, 254);
                    } else if (outsideOriginalBackgroundColor == Qt::black) {
                        outsideOriginalBackgroundColor = QColor(1, 1, 1);
                    }
                    fillMarginsInPlace(original_background,
                                       normalize_illumination_crop_area,
                                       outsideOriginalBackgroundColor);
                    original_background_dst.fill(outsideOriginalBackgroundColor);

                    if (!contentRect.isEmpty()) {
                        const QRect src_rect(contentRect.translated(-small_margins_rect.topLeft()));
                        const QRect dst_rect(contentRect);
                        drawOver(original_background_dst, dst_rect, original_background, src_rect);
                    }
                    original_background = QImage();

                    status.throwIfCancelled();

                    splitImage->setOriginalBackgroundImage(original_background_dst);
                }

                combineMixed(maybe_normalized, bw_content, bw_mask_filled, !render_params.splitOutput());
            }
        }

        status.throwIfCancelled();

        assert(!target_size.isEmpty());
        QImage dst(target_size, maybe_normalized.format());

        if (maybe_normalized.format() == QImage::Format_Indexed8) {
            dst.setColorTable(createGrayscalePalette());
        }

        if (dst.isNull()) {
            // Both the constructor and setColorTable() above can leave the image null.
            throw std::bad_alloc();
        }

        if (m_colorParams.colorCommonOptions().getFillingColor() == ColorCommonOptions::WHITE) {
            outsideBackgroundColor = Qt::white;
        } else if (render_params.needBinarization()) {
            outsideBackgroundColor = outsideBackgroundColorBW;
        }
        fillMarginsInPlace(maybe_normalized, normalize_illumination_crop_area, outsideBackgroundColor);
        dst.fill(outsideBackgroundColor);

        if (!contentRect.isEmpty()) {
            const QRect src_rect(contentRect.translated(-small_margins_rect.topLeft()));
            const QRect dst_rect(contentRect);
            drawOver(dst, dst_rect, maybe_normalized, src_rect);
        }
        maybe_normalized = QImage();

        if (render_params.mixedOutput() && render_params.needBinarization()) {
            applyFillZonesToMixedInPlace(dst, fill_zones, picture_mask_output);
        } else {
            applyFillZonesInPlace(dst, fill_zones);
        }

        status.throwIfCancelled();

        if (render_params.splitOutput()) {
            splitImage->setMask(picture_mask_output, render_params.needBinarization());
            splitImage->setBackgroundImage(dst);

            return QImage();
        }

        return dst;
    }  // OutputGenerator::processWithoutDewarping

    QImage OutputGenerator::processWithDewarping(const TaskStatus& status,
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
                                                 SplitImage* splitImage) {
        const RenderParams render_params(m_colorParams, m_splittingOptions);

        const QSize target_size(m_outRect.size().expandedTo(QSize(1, 1)));
        if (m_contentRect.isEmpty()) {
            QImage emptyImage(BinaryImage(target_size, WHITE).toQImage());
            if (!render_params.splitOutput()) {
                return emptyImage;
            } else {
                splitImage->setForegroundImage(emptyImage);
                splitImage->setBackgroundImage(emptyImage.convertToFormat(QImage::Format_Indexed8));
                return QImage();
            }
        }

        QRect contentRect = m_xform.resultingPreCropArea().boundingRect().toRect().intersected(
                render_params.cutMargins() ? m_contentRect : m_outRect);

        // The whole image minus the part cut off by the split line.
        const QRect big_margins_rect(
                m_xform.resultingPreCropArea().boundingRect().toRect()
        );

        // For various reasons, we need some whitespace around the content
        // area.  This is the number of pixels of such whitespace.
        const int content_margin = m_dpi.vertical() * 20 / 300;

        // The content area (in output image coordinates) extended
        // with content_margin.  Note that we prevent that extension
        // from reaching the neighboring page.
        const QRect small_margins_rect(
                contentRect.adjusted(
                        -content_margin, -content_margin,
                        content_margin, content_margin
                ).intersected(big_margins_rect)
        );

        // This is the area we are going to pass to estimateBackground().
        // estimateBackground() needs some margins around content, and
        // generally smaller margins are better, except when there is
        // some garbage that connects the content to the edge of the
        // image area.
        const QRect normalize_illumination_rect(
                small_margins_rect
        );

        // Crop area in original image coordinates.
        const QPolygonF orig_image_crop_area(
                m_xform.transformBack().map(m_xform.resultingPreCropArea())
        );

        // Crop area in maybe_normalized image coordinates.
        QPolygonF normalize_illumination_crop_area(m_xform.resultingPreCropArea());
        normalize_illumination_crop_area.translate(-normalize_illumination_rect.topLeft());

        const bool color_original = !input.origImage().allGray();

        QColor outsideBackgroundColor;
        QColor outsideBackgroundColorBW;

        const bool needNormalizeIllumination
                = (render_params.normalizeIllumination() && render_params.needBinarization())
                  || (render_params.normalizeIlluminationColor() && !render_params.needBinarization());

        // Original image, but:
        // 1. In a format we can handle, that is grayscale, RGB32, ARGB32
        // 2. With illumination normalized over the content area, if required.
        // 3. With margins filled with white, if required.
        QImage normalized_original;

        // The output we would get if dewarping was turned off, except always grayscale.
        // Used for automatic picture detection and binarization threshold calculation.
        // This image corresponds to the area of normalize_illumination_rect above.
        GrayImage warped_gray_output;
        // Picture mask (white indicate a picture) in the same coordinates as
        // warped_gray_output.  Only built for Mixed mode.
        BinaryImage warped_bw_mask;

        BinaryThreshold bw_threshold(128);

        const QTransform norm_illum_to_original(
                QTransform().translate(
                        normalize_illumination_rect.left(),
                        normalize_illumination_rect.top()
                ) * m_xform.transformBack()
        );

        if (!needNormalizeIllumination) {
            if (color_original) {
                normalized_original = convertToRGBorRGBA(input.origImage());
            } else {
                normalized_original = input.grayImage();
            }
            warped_gray_output = transformToGray(
                    input.grayImage(), m_xform.transform(), normalize_illumination_rect,
                    OutsidePixels::assumeWeakColor(Qt::white)
            );
        } else {
            GrayImage warped_gray_background;
            warped_gray_output = normalizeIlluminationGray(
                    status, input.grayImage(), orig_image_crop_area,
                    m_xform.transform(), normalize_illumination_rect,
                    &warped_gray_background, dbg
            );

            status.throwIfCancelled();

            // Transform warped_gray_background to original image coordinates.
            warped_gray_background = transformToGray(
                    warped_gray_background.toQImage(), norm_illum_to_original,
                    input.origImage().rect(), OutsidePixels::assumeWeakColor(Qt::black)
            );
            if (dbg) {
                dbg->add(warped_gray_background, "orig_background");
            }

            status.throwIfCancelled();
            // Turn background into a grayscale, illumination-normalized image.
            grayRasterOp<RaiseAboveBackground>(warped_gray_background, input.grayImage());
            if (dbg) {
                dbg->add(warped_gray_background, "norm_illum_gray");
            }

            status.throwIfCancelled();

            if (!color_original || render_params.binaryOutput()) {
                normalized_original = warped_gray_background;
            } else {
                normalized_original = convertToRGBorRGBA(input.origImage());
                adjustBrightnessGrayscale(normalized_original, warped_gray_background);
                if (dbg) {
                    dbg->add(normalized_original, "norm_illum_color");
                }
            }
        }

        status.throwIfCancelled();

        if (render_params.binaryOutput()) {
            bw_threshold = calcBinarizationThreshold(
                    warped_gray_output, normalize_illumination_crop_area
            );

            status.throwIfCancelled();
        } else if (render_params.mixedOutput()) {
            warped_bw_mask = BinaryImage(small_margins_rect.size(), BLACK);

            if ((m_pictureShapeOptions.getPictureShape() != RECTANGULAR_SHAPE)
                || !m_outputProcessingParams.isAutoZonesFound()) {
                if (m_pictureShapeOptions.getPictureShape() != OFF_SHAPE) {
                    warped_bw_mask = estimateBinarizationMask(
                            status, GrayImage(warped_gray_output),
                            normalize_illumination_rect,
                            small_margins_rect, dbg
                    );
                    if (dbg) {
                        dbg->add(warped_bw_mask, "warped_bw_mask");
                    }
                }

                removeAutoPictureZones(picture_zones);
                p_settings->setPictureZones(p_pageId, picture_zones);
                m_outputProcessingParams.setAutoZonesFound(false);
                p_settings->setOutputProcessingParams(p_pageId, m_outputProcessingParams);
            }
            if ((m_pictureShapeOptions.getPictureShape() == RECTANGULAR_SHAPE)
                && !m_outputProcessingParams.isAutoZonesFound()) {
                std::vector<QRect> areas;
                warped_bw_mask.rectangularizeAreas(areas, WHITE, m_pictureShapeOptions.getSensitivity());

                QTransform xform1(m_xform.transform());
                xform1 *= QTransform().translate(-normalize_illumination_rect.x(), -normalize_illumination_rect.y());
                QTransform inv_xform(xform1.inverted());

                for (auto i : areas) {
                    QRectF area0(i);
                    QPolygonF area1(area0);
                    QPolygonF area(inv_xform.map(area1));

                    Zone zone1(area);

                    picture_zones.add(zone1);
                }
                p_settings->setPictureZones(p_pageId, picture_zones);
                m_outputProcessingParams.setAutoZonesFound(true);
                p_settings->setOutputProcessingParams(p_pageId, m_outputProcessingParams);

                warped_bw_mask.fill(BLACK);
            }

            status.throwIfCancelled();

            if (auto_picture_mask) {
                if (auto_picture_mask->size() != target_size) {
                    BinaryImage(target_size).swap(*auto_picture_mask);
                }
                auto_picture_mask->fill(BLACK);

                if (!contentRect.isEmpty()) {
                    const QRect src_rect(contentRect.translated(-small_margins_rect.topLeft()));
                    const QRect dst_rect(contentRect);
                    rasterOp<RopSrc>(*auto_picture_mask, dst_rect, warped_bw_mask, src_rect.topLeft());
                }
            }

            status.throwIfCancelled();

            modifyBinarizationMask(warped_bw_mask, small_margins_rect, picture_zones);
            if (dbg) {
                dbg->add(warped_bw_mask, "warped_bw_mask with zones");
            }

            status.throwIfCancelled();

            // For Mixed output, we mask out pictures when calculating binarization threshold.
            bw_threshold = calcBinarizationThreshold(
                    warped_gray_output, normalize_illumination_crop_area, &warped_bw_mask
            );

            status.throwIfCancelled();
        }

        if (m_dewarpingOptions.dewarpingMode() == AUTO) {
            DistortionModelBuilder model_builder(Vec2d(0, 1));

            const QRect content_rect(
                    contentRect.translated(-normalize_illumination_rect.topLeft())
            );
            TextLineTracer::trace(
                    warped_gray_output, m_dpi, content_rect, model_builder, status, dbg
            );
            model_builder.transform(norm_illum_to_original);

            TopBottomEdgeTracer::trace(
                    input.grayImage(), model_builder.verticalBounds(),
                    model_builder, status, dbg
            );

            distortion_model = model_builder.tryBuildModel(dbg, &input.grayImage().toQImage());
            if (!distortion_model.isValid()) {
                setupTrivialDistortionModel(distortion_model);
            }

            BinaryImage bw_image(input.grayImage(), BinaryThreshold(64));

            QTransform transform = m_xform.preRotation().transform(bw_image.size());
            QTransform inv_transform = transform.inverted();

            int degrees = m_xform.preRotation().toDegrees();
            bw_image = orthogonalRotation(bw_image, degrees);

            const std::vector<QPointF>& top_polyline0 = distortion_model.topCurve().polyline();
            const std::vector<QPointF>& bottom_polyline0 = distortion_model.bottomCurve().polyline();

            std::vector<QPointF> top_polyline;
            std::vector<QPointF> bottom_polyline;

            for (auto i : top_polyline0) {
                top_polyline.push_back(transform.map(i));
            }

            for (auto i : bottom_polyline0) {
                bottom_polyline.push_back(transform.map(i));
            }


            const PageId& pageId = p_pageId;

            QString stAngle;

            float max_angle = 2.75;

            if ((pageId.subPage() == PageId::SINGLE_PAGE) || (pageId.subPage() == PageId::LEFT_PAGE)) {
                float vert_skew_angle_left = vert_border_skew_angle(top_polyline.front(), bottom_polyline.front());

                stAngle.setNum(vert_skew_angle_left);


                if (vert_skew_angle_left > max_angle) {
                    auto top_x = static_cast<float>(top_polyline.front().x());
                    auto bottom_x = static_cast<float>(bottom_polyline.front().x());

                    if (top_x < bottom_x) {
                        std::vector<QPointF> new_bottom_polyline;

                        QPointF pt(top_x, bottom_polyline.front().y());

                        new_bottom_polyline.push_back(pt);

                        for (auto i : bottom_polyline) {
                            new_bottom_polyline.push_back(inv_transform.map(i));
                        }

                        distortion_model.setBottomCurve(dewarping::Curve(new_bottom_polyline));
                    } else {
                        std::vector<QPointF> new_top_polyline;

                        QPointF pt(bottom_x, top_polyline.front().y());

                        new_top_polyline.push_back(pt);

                        for (auto i : top_polyline) {
                            new_top_polyline.push_back(inv_transform.map(i));
                        }

                        distortion_model.setBottomCurve(dewarping::Curve(new_top_polyline));
                    }
                }
            } else {
                float vert_skew_angle_right = vert_border_skew_angle(top_polyline.back(), bottom_polyline.back());

                stAngle.setNum(vert_skew_angle_right);


                if (vert_skew_angle_right > max_angle) {
                    auto top_x = static_cast<float>(top_polyline.back().x());
                    auto bottom_x = static_cast<float>(bottom_polyline.back().x());

                    if (top_x > bottom_x) {
                        std::vector<QPointF> new_bottom_polyline;

                        QPointF pt(top_x, bottom_polyline.back().y());

                        for (auto i : bottom_polyline) {
                            new_bottom_polyline.push_back(inv_transform.map(i));
                        }

                        new_bottom_polyline.push_back(pt);

                        distortion_model.setBottomCurve(dewarping::Curve(new_bottom_polyline));
                    } else {
                        std::vector<QPointF> new_top_polyline;

                        QPointF pt(bottom_x, top_polyline.back().y());

                        for (auto i : top_polyline) {
                            new_top_polyline.push_back(inv_transform.map(i));
                        }

                        new_top_polyline.push_back(pt);

                        distortion_model.setBottomCurve(dewarping::Curve(new_top_polyline));
                    }
                }
            }
        } else if (m_dewarpingOptions.dewarpingMode() == MARGINAL) {
            BinaryImage bw_image(input.grayImage(), BinaryThreshold(64));

            QTransform transform = m_xform.preRotation().transform(bw_image.size());
            QTransform inv_transform = transform.inverted();

            int degrees = m_xform.preRotation().toDegrees();
            bw_image = orthogonalRotation(bw_image, degrees);

            setupTrivialDistortionModel(distortion_model);

            const PageId& pageId = p_pageId;

            int max_red_points = 5;
            XSpline top_spline;

            const std::vector<QPointF>& top_polyline = distortion_model.topCurve().polyline();

            const QLineF top_line(transform.map(top_polyline.front()), transform.map(top_polyline.back()));

            top_spline.appendControlPoint(top_line.p1(), 0);

            if ((pageId.subPage() == PageId::SINGLE_PAGE) || (pageId.subPage() == PageId::LEFT_PAGE)) {
                for (int i = 29 - max_red_points; i < 29; i++) {
                    top_spline.appendControlPoint(top_line.pointAt((float) i / 29.0), 1);
                }
            } else {
                for (int i = 1; i <= max_red_points; i++) {
                    top_spline.appendControlPoint(top_line.pointAt((float) i / 29.0), 1);
                }
            }

            top_spline.appendControlPoint(top_line.p2(), 0);

            for (int i = 0; i <= top_spline.numSegments(); i++) {
                movePointToTopMargin(bw_image, top_spline, i);
            }

            for (int i = 0; i <= top_spline.numSegments(); i++) {
                top_spline.moveControlPoint(i, inv_transform.map(top_spline.controlPointPosition(i)));
            }

            distortion_model.setTopCurve(dewarping::Curve(top_spline));


            XSpline bottom_spline;

            const std::vector<QPointF>& bottom_polyline = distortion_model.bottomCurve().polyline();

            const QLineF bottom_line(transform.map(bottom_polyline.front()), transform.map(bottom_polyline.back()));

            bottom_spline.appendControlPoint(bottom_line.p1(), 0);

            if ((pageId.subPage() == PageId::SINGLE_PAGE) || (pageId.subPage() == PageId::LEFT_PAGE)) {
                for (int i = 29 - max_red_points; i < 29; i++) {
                    bottom_spline.appendControlPoint(top_line.pointAt((float) i / 29.0), 1);
                }
            } else {
                for (int i = 1; i <= max_red_points; i++) {
                    bottom_spline.appendControlPoint(top_line.pointAt((float) i / 29.0), 1);
                }
            }

            bottom_spline.appendControlPoint(bottom_line.p2(), 0);

            for (int i = 0; i <= bottom_spline.numSegments(); i++) {
                movePointToBottomMargin(bw_image, bottom_spline, i);
            }

            for (int i = 0; i <= bottom_spline.numSegments(); i++) {
                bottom_spline.moveControlPoint(i, inv_transform.map(bottom_spline.controlPointPosition(i)));
            }

            distortion_model.setBottomCurve(dewarping::Curve(bottom_spline));

            if (!distortion_model.isValid()) {
                setupTrivialDistortionModel(distortion_model);
            }

            if (dbg) {
                QImage out_image(bw_image.toQImage().convertToFormat(QImage::Format_RGB32));
                for (int i = 0; i <= top_spline.numSegments(); i++) {
                    drawPoint(out_image, top_spline.controlPointPosition(i));
                }
                for (int i = 0; i <= bottom_spline.numSegments(); i++) {
                    drawPoint(out_image, bottom_spline.controlPointPosition(i));
                }
                dbg->add(out_image, "marginal dewarping");
            }
        }
        warped_gray_output = GrayImage();  // Save memory.

        status.throwIfCancelled();

        {
            QImage cropped_image = transform(
                    normalized_original, m_xform.transform(),
                    contentRect, OutsidePixels::assumeColor(Qt::white)
            );
            outsideBackgroundColor = BackgroundColorCalculator::calcDominantBackgroundColor(cropped_image);
            if (render_params.needBinarization()) {
                outsideBackgroundColorBW = BackgroundColorCalculator::calcDominantBackgroundColorBW(cropped_image);

                if (!m_outputProcessingParams.isWhiteOnBlackAutoDetected()) {
                    m_outputProcessingParams.setWhiteOnBlackMode(outsideBackgroundColorBW == Qt::black);

                    m_outputProcessingParams.setWhiteOnBlackAutoDetected(true);
                    p_settings->setOutputProcessingParams(p_pageId, m_outputProcessingParams);
                }
            }
        }

        status.throwIfCancelled();

        QImage dewarped;
        try {
            dewarped = dewarp(
                    QTransform(), normalized_original, m_xform.transform(),
                    distortion_model, depth_perception, outsideBackgroundColor
            );
        } catch (const std::runtime_error&) {
            // Probably an impossible distortion model.  Let's fall back to a trivial one.
            setupTrivialDistortionModel(distortion_model);
            dewarped = dewarp(
                    QTransform(), normalized_original, m_xform.transform(),
                    distortion_model, depth_perception, outsideBackgroundColor
            );
        }

        normalized_original = QImage();  // Save memory.
        if (dbg) {
            dbg->add(dewarped, "dewarped");
        }

        status.throwIfCancelled();

        std::shared_ptr<DewarpingPointMapper> mapper(
                new DewarpingPointMapper(
                        distortion_model, depth_perception.value(),
                        m_xform.transform(), contentRect
                )
        );
        boost::function<QPointF(const QPointF&)> const orig_to_output(
                boost::bind(&DewarpingPointMapper::mapToDewarpedSpace, mapper, _1)
        );

        double deskew_angle = maybe_deskew(&dewarped, m_dewarpingOptions, outsideBackgroundColor);

        {
            QTransform post_rotate;

            QPointF center(m_outRect.width() / 2, m_outRect.height() / 2);

            post_rotate.translate(center.x(), center.y());
            post_rotate.rotate(-deskew_angle);
            post_rotate.translate(-center.x(), -center.y());

            postTransform = post_rotate;
        }

        BinaryImage dewarping_content_area_mask(input.grayImage().size(), BLACK);
        QPolygonF content_area = orig_image_crop_area;
        if (render_params.cutMargins()) {
            content_area = content_area.intersected(m_xform.transformBack().map(QRectF(contentRect)));
        }
        fillMarginsInPlace(dewarping_content_area_mask, content_area, WHITE);
        QImage dewarping_content_area_mask_dewarped(
                dewarp(
                        QTransform(), dewarping_content_area_mask.toQImage(), m_xform.transform(),
                        distortion_model, depth_perception, Qt::white
                )
        );
        deskew(&dewarping_content_area_mask_dewarped, deskew_angle, Qt::white);
        dewarping_content_area_mask = BinaryImage(dewarping_content_area_mask_dewarped);
        dewarping_content_area_mask_dewarped = QImage();

        fillMarginsInPlace(dewarped, dewarping_content_area_mask, outsideBackgroundColor);

        if (render_params.binaryOutput()) {
            QImage dewarped_and_maybe_smoothed;
            // We only do smoothing if we are going to do binarization later.
            if (!render_params.needSavitzkyGolaySmoothing()) {
                dewarped_and_maybe_smoothed = dewarped;
            } else {
                dewarped_and_maybe_smoothed = smoothToGrayscale(dewarped, m_dpi);
                if (dbg) {
                    dbg->add(dewarped_and_maybe_smoothed, "smoothed");
                }
            }
            dewarped = QImage();

            status.throwIfCancelled();

            BinaryImage dewarped_bw_content(
                    binarize(dewarped_and_maybe_smoothed, dewarping_content_area_mask)
            );

            status.throwIfCancelled();

            if (m_colorParams.colorCommonOptions().getFillingColor() == ColorCommonOptions::BACKGROUND) {
                fillMarginsInPlace(dewarped_bw_content, dewarping_content_area_mask,
                                   (outsideBackgroundColorBW == Qt::black) ? BLACK : WHITE);
            }

            dewarped_and_maybe_smoothed = QImage();  // Save memory.
            if (dbg) {
                dbg->add(dewarped_bw_content, "dewarped_bw_content");
            }

            status.throwIfCancelled();

            if (render_params.needMorphologicalSmoothing()) {
                morphologicalSmoothInPlace(dewarped_bw_content, status);
                if (dbg) {
                    dbg->add(dewarped_bw_content, "edges_smoothed");
                }
            }

            status.throwIfCancelled();

            // It's important to keep despeckling the very last operation
            // affecting the binary part of the output. That's because
            // we will be reconstructing the input to this despeckling
            // operation from the final output file.
            maybeDespeckleInPlace(
                    dewarped_bw_content, m_outRect, m_outRect, m_despeckleLevel,
                    speckles_image, m_dpi, status, dbg
            );

            applyFillZonesInPlace(dewarped_bw_content, fill_zones, orig_to_output, postTransform);

            return dewarped_bw_content.toQImage();
        }

        BinaryImage dewarped_picture_mask;
        if (render_params.mixedOutput()) {
            const QTransform orig_to_small_margins(
                    m_xform.transform() * QTransform().translate(
                            -small_margins_rect.left(),
                            -small_margins_rect.top()
                    )
            );
            QTransform small_margins_to_output;
            small_margins_to_output.translate(
                    small_margins_rect.left(), small_margins_rect.top()
            );
            BinaryImage dewarped_bw_mask(
                    dewarp(
                            orig_to_small_margins, warped_bw_mask.toQImage(),
                            small_margins_to_output, distortion_model,
                            depth_perception, Qt::black
                    )
            );
            warped_bw_mask.release();
            if (dbg) {
                dbg->add(dewarped_bw_mask, "dewarped_bw_mask");
            }

            status.throwIfCancelled();

            QImage dewarped_bw_mask_deskewed(dewarped_bw_mask.toQImage());
            deskew(&dewarped_bw_mask_deskewed, deskew_angle, Qt::black);
            dewarped_bw_mask = BinaryImage(dewarped_bw_mask_deskewed);
            dewarped_bw_mask_deskewed = QImage();

            dewarped_picture_mask = dewarped_bw_mask;

            if (render_params.needBinarization()) {
                QImage dewarped_and_maybe_smoothed;
                if (!render_params.needSavitzkyGolaySmoothing()) {
                    dewarped_and_maybe_smoothed = dewarped;
                } else {
                    dewarped_and_maybe_smoothed = smoothToGrayscale(dewarped, m_dpi);
                    if (dbg) {
                        dbg->add(dewarped_and_maybe_smoothed, "smoothed");
                    }
                }

                status.throwIfCancelled();

                BinaryImage dewarped_bw_mask_filled(dewarped_bw_mask);
                fillMarginsInPlace(dewarped_bw_mask_filled, dewarping_content_area_mask, WHITE);

                BinaryImage dewarped_bw_content(
                        binarize(dewarped_and_maybe_smoothed, dewarped_bw_mask_filled)
                );

                dewarped_bw_mask.release();
                dewarped_and_maybe_smoothed = QImage();  // Save memory.

                if (dbg) {
                    dbg->add(dewarped_bw_content, "dewarped_bw_content");
                }

                status.throwIfCancelled();

                if (render_params.needMorphologicalSmoothing()) {
                    morphologicalSmoothInPlace(dewarped_bw_content, status);
                    if (dbg) {
                        dbg->add(dewarped_bw_content, "edges_smoothed");
                    }
                }

                status.throwIfCancelled();

                if (render_params.needMorphologicalSmoothing()) {
                    morphologicalSmoothInPlace(dewarped_bw_content, status);
                    if (dbg) {
                        dbg->add(dewarped_bw_content, "edges_smoothed");
                    }
                }

                status.throwIfCancelled();

                // It's important to keep despeckling the very last operation
                // affecting the binary part of the output. That's because
                // we will be reconstructing the input to this despeckling
                // operation from the final output file.
                maybeDespeckleInPlace(
                        dewarped_bw_content, m_outRect, contentRect,
                        m_despeckleLevel, speckles_image, m_dpi, status, dbg
                );

                status.throwIfCancelled();

                if (needNormalizeIllumination && !render_params.normalizeIlluminationColor()) {
                    QImage orig_without_illumination;
                    if (color_original) {
                        orig_without_illumination
                                = convertToRGBorRGBA(input.origImage());
                    } else {
                        orig_without_illumination = input.grayImage();
                    }

                    status.throwIfCancelled();

                    try {
                        dewarped = dewarp(
                                QTransform(), orig_without_illumination,
                                m_xform.transform(),
                                distortion_model, depth_perception, outsideBackgroundColor
                        );
                    } catch (const std::runtime_error&) {
                        setupTrivialDistortionModel(distortion_model);
                        dewarped = dewarp(
                                QTransform(), orig_without_illumination,
                                m_xform.transform(),
                                distortion_model, depth_perception, outsideBackgroundColor
                        );
                    }
                    orig_without_illumination = QImage();

                    deskew(&dewarped, deskew_angle, outsideBackgroundColor);

                    status.throwIfCancelled();
                }

                if (render_params.originalBackground()) {
                    QImage original_background(dewarped);
                    // reserving original black and white colors
                    reserveBlackAndWhite(original_background);
                    // subtracting bw content filling the content areas with white,
                    // considering the colors of the content and background
                    BinaryImage dewarped_bw_content_mask(dewarped_bw_content);
                    // we applying BW filling zones to the mask to create an ability to control
                    // the content of original background manually
                    applyFillZonesInPlace(dewarped_bw_content_mask, fill_zones, orig_to_output, postTransform);
                    if (outsideBackgroundColorBW == Qt::white) {
                        dewarped_bw_content_mask.invert();
                    }
                    applyMask(original_background,
                              dewarped_bw_content_mask,
                              WHITE);
                    // applying picture mask filling the picture areas with black
                    applyMask(original_background, dewarped_bw_mask_filled, BLACK);

                    status.throwIfCancelled();

                    QColor outsideOriginalBackgroundColor = outsideBackgroundColor;
                    if (m_colorParams.colorCommonOptions().getFillingColor() == ColorCommonOptions::WHITE) {
                        outsideOriginalBackgroundColor = Qt::white;
                    }
                    // reserving outside background color if it's black or white
                    if (outsideOriginalBackgroundColor == Qt::white) {
                        outsideOriginalBackgroundColor = QColor(254, 254, 254);
                    } else if (outsideOriginalBackgroundColor == Qt::black) {
                        outsideOriginalBackgroundColor = QColor(1, 1, 1);
                    }

                    fillMarginsInPlace(original_background,
                                       dewarping_content_area_mask,
                                       outsideOriginalBackgroundColor);

                    splitImage->setOriginalBackgroundImage(original_background);
                }

                combineMixed(dewarped, dewarped_bw_content, dewarped_bw_mask_filled, !render_params.splitOutput());
            }
        }

        if (m_colorParams.colorCommonOptions().getFillingColor() == ColorCommonOptions::WHITE) {
            outsideBackgroundColor = Qt::white;
        } else if (render_params.needBinarization()) {
            outsideBackgroundColor = outsideBackgroundColorBW;
        }
        fillMarginsInPlace(dewarped, dewarping_content_area_mask, outsideBackgroundColor);

        if (render_params.mixedOutput() && render_params.needBinarization()) {
            applyFillZonesToMixedInPlace(dewarped, fill_zones, orig_to_output, postTransform, dewarped_picture_mask);
        } else {
            applyFillZonesInPlace(dewarped, fill_zones, orig_to_output, postTransform);
        }

        status.throwIfCancelled();

        if (render_params.splitOutput()) {
            splitImage->setMask(dewarped_picture_mask, render_params.needBinarization());
            splitImage->setBackgroundImage(dewarped);

            return QImage();
        }

        return dewarped;
    }  // OutputGenerator::processWithDewarping

    /**
     * Set up a distortion model corresponding to the content rect,
     * which will result in no distortion correction.
     */
    void OutputGenerator::setupTrivialDistortionModel(DistortionModel& distortion_model) const {
        QPolygonF poly;
        if (!m_contentRect.isEmpty()) {
            poly = QRectF(m_contentRect);
        } else {
            poly << m_contentRect.topLeft() + QPointF(-0.5, -0.5);
            poly << m_contentRect.topLeft() + QPointF(0.5, -0.5);
            poly << m_contentRect.topLeft() + QPointF(0.5, 0.5);
            poly << m_contentRect.topLeft() + QPointF(-0.5, 0.5);
        }
        poly = m_xform.transformBack().map(poly);

        std::vector<QPointF> top_polyline, bottom_polyline;
        top_polyline.push_back(poly[0]);  // top-left
        top_polyline.push_back(poly[1]);  // top-right
        bottom_polyline.push_back(poly[3]);  // bottom-left
        bottom_polyline.push_back(poly[2]);  // bottom-right
        distortion_model.setTopCurve(Curve(top_polyline));
        distortion_model.setBottomCurve(Curve(bottom_polyline));
    }

    CylindricalSurfaceDewarper OutputGenerator::createDewarper(const DistortionModel& distortion_model,
                                                               const QTransform& distortion_model_to_target,
                                                               double depth_perception) {
        if (distortion_model_to_target.isIdentity()) {
            return CylindricalSurfaceDewarper(
                    distortion_model.topCurve().polyline(),
                    distortion_model.bottomCurve().polyline(), depth_perception
            );
        }

        std::vector<QPointF> top_polyline(distortion_model.topCurve().polyline());
        std::vector<QPointF> bottom_polyline(distortion_model.bottomCurve().polyline());
        for (QPointF& pt : top_polyline) {
            pt = distortion_model_to_target.map(pt);
        }
        for (QPointF& pt : bottom_polyline) {
            pt = distortion_model_to_target.map(pt);
        }

        return CylindricalSurfaceDewarper(
                top_polyline, bottom_polyline, depth_perception
        );
    }

/**
 * \param orig_to_src Transformation from the original image coordinates
 *                    to the coordinate system of \p src image.
 * \param src_to_output Transformation from the \p src image coordinates
 *                      to output image coordinates.
 * \param distortion_model Distortion model.
 * \param depth_perception Depth perception.
 * \param bg_color The color to use for areas outsize of \p src.
 * \param modified_content_rect A vertically shrunk version of outputContentRect().
 *                              See function definition for more details.
 */
    QImage OutputGenerator::dewarp(const QTransform& orig_to_src,
                                   const QImage& src,
                                   const QTransform& src_to_output,
                                   const DistortionModel& distortion_model,
                                   const DepthPerception& depth_perception,
                                   const QColor& bg_color) const {
        const CylindricalSurfaceDewarper dewarper(
                createDewarper(distortion_model, orig_to_src, depth_perception.value())
        );

        // Model domain is a rectangle in output image coordinates that
        // will be mapped to our curved quadrilateral.
        const QRect model_domain(
                distortion_model.modelDomain(
                        dewarper, orig_to_src * src_to_output, outputContentRect()
                ).toRect()
        );
        if (model_domain.isEmpty()) {
            GrayImage out(src.size());
            out.fill(0xff);  // white

            return out;
        }

        return RasterDewarper::dewarp(
                src, m_outRect.size(), dewarper, model_domain, bg_color
        );
    }

    QSize OutputGenerator::from300dpi(const QSize& size, const Dpi& target_dpi) {
        const double hscale = target_dpi.horizontal() / 300.0;
        const double vscale = target_dpi.vertical() / 300.0;
        const int width = qRound(size.width() * hscale);
        const int height = qRound(size.height() * vscale);

        return QSize(std::max(1, width), std::max(1, height));
    }

    QSize OutputGenerator::to300dpi(const QSize& size, const Dpi& source_dpi) {
        const double hscale = 300.0 / source_dpi.horizontal();
        const double vscale = 300.0 / source_dpi.vertical();
        const int width = qRound(size.width() * hscale);
        const int height = qRound(size.height() * vscale);

        return QSize(std::max(1, width), std::max(1, height));
    }

    QImage OutputGenerator::convertToRGBorRGBA(const QImage& src) {
        const QImage::Format fmt = src.hasAlphaChannel()
                                   ? QImage::Format_ARGB32 : QImage::Format_RGB32;

        return src.convertToFormat(fmt);
    }

    void OutputGenerator::fillMarginsInPlace(QImage& image, const QPolygonF& content_poly, const QColor& color) {
        if ((image.format() == QImage::Format_Mono) || (image.format() == QImage::Format_MonoLSB)) {
            BinaryImage binaryImage(image);
            PolygonRasterizer::fillExcept(
                    binaryImage, (color == Qt::black) ? BLACK : WHITE, content_poly, Qt::WindingFill
            );
            image = binaryImage.toQImage();

            return;
        }

        if ((image.format() == QImage::Format_Indexed8) && image.isGrayscale()) {
            PolygonRasterizer::grayFillExcept(
                    image, static_cast<unsigned char>(qGray(color.rgb())), content_poly, Qt::WindingFill
            );

            return;
        }

        assert(image.format() == QImage::Format_RGB32 || image.format() == QImage::Format_ARGB32);

        const QImage::Format imageFormat = image.format();
        image = image.convertToFormat(QImage::Format_ARGB32_Premultiplied);

        {
            QPainter painter(&image);
            painter.setRenderHint(QPainter::Antialiasing, true);
            painter.setBrush(color);
            painter.setPen(Qt::NoPen);

            QPainterPath outer_path;
            outer_path.addRect(image.rect());
            QPainterPath inner_path;
            inner_path.addPolygon(content_poly);

            painter.drawPath(outer_path.subtracted(inner_path));
        }

        image = image.convertToFormat(imageFormat);
    }      // OutputGenerator::fillMarginsInPlace

    void OutputGenerator::fillMarginsInPlace(BinaryImage& image, const QPolygonF& content_poly, const BWColor& color) {
        PolygonRasterizer::fillExcept(
                image, color, content_poly, Qt::WindingFill
        );
    }

    void OutputGenerator::fillMarginsInPlace(QImage& image, const BinaryImage& content_mask, const QColor& color) {
        if ((image.format() == QImage::Format_Mono) || (image.format() == QImage::Format_MonoLSB)) {
            BinaryImage binaryImage(image);
            fillExcept(binaryImage, content_mask, (color == Qt::black) ? BLACK : WHITE);
            image = binaryImage.toQImage();

            return;
        }

        if (image.format() == QImage::Format_Indexed8) {
            fillExcept<uint8_t>(image, content_mask, color);
        } else {
            assert(image.format() == QImage::Format_RGB32 || image.format() == QImage::Format_ARGB32);

            fillExcept<uint32_t>(image, content_mask, color);
        }
    }

    void
    OutputGenerator::fillMarginsInPlace(BinaryImage& image, const BinaryImage& content_mask, const BWColor& color) {
        fillExcept(image, content_mask, color);
    }

    GrayImage OutputGenerator::detectPictures(const GrayImage& input_300dpi,
                                              const TaskStatus& status,
                                              DebugImages* const dbg) {
        // We stretch the range of gray levels to cover the whole
        // range of [0, 255].  We do it because we want text
        // and background to be equally far from the center
        // of the whole range.  Otherwise text printed with a big
        // font will be considered a picture.
        GrayImage stretched(stretchGrayRange(input_300dpi, 0.01, 0.01));
        if (dbg) {
            dbg->add(stretched, "stretched");
        }

        status.throwIfCancelled();

        GrayImage eroded(erodeGray(stretched, QSize(3, 3), 0x00));
        if (dbg) {
            dbg->add(eroded, "eroded");
        }

        status.throwIfCancelled();

        GrayImage dilated(dilateGray(stretched, QSize(3, 3), 0xff));
        if (dbg) {
            dbg->add(dilated, "dilated");
        }

        stretched = GrayImage();  // Save memory.
        status.throwIfCancelled();

        grayRasterOp<CombineInverted>(dilated, eroded);
        GrayImage gray_gradient(dilated);
        dilated = GrayImage();
        eroded = GrayImage();
        if (dbg) {
            dbg->add(gray_gradient, "gray_gradient");
        }

        status.throwIfCancelled();

        GrayImage marker(erodeGray(gray_gradient, QSize(35, 35), 0x00));
        if (dbg) {
            dbg->add(marker, "marker");
        }

        status.throwIfCancelled();

        seedFillGrayInPlace(marker, gray_gradient, CONN8);
        GrayImage reconstructed(marker);
        marker = GrayImage();
        if (dbg) {
            dbg->add(reconstructed, "reconstructed");
        }

        status.throwIfCancelled();

        grayRasterOp<GRopInvert<GRopSrc>>(reconstructed, reconstructed);
        if (dbg) {
            dbg->add(reconstructed, "reconstructed_inverted");
        }

        status.throwIfCancelled();

        GrayImage holes_filled(createFramedImage(reconstructed.size()));
        seedFillGrayInPlace(holes_filled, reconstructed, CONN8);
        reconstructed = GrayImage();
        if (dbg) {
            dbg->add(holes_filled, "holes_filled");
        }

        return holes_filled;
    }  // OutputGenerator::detectPictures

    QImage OutputGenerator::smoothToGrayscale(const QImage& src, const Dpi& dpi) {
        const int min_dpi = std::min(dpi.horizontal(), dpi.vertical());
        int window;
        int degree;
        if (min_dpi <= 200) {
            window = 5;
            degree = 3;
        } else if (min_dpi <= 400) {
            window = 7;
            degree = 4;
        } else if (min_dpi <= 800) {
            window = 11;
            degree = 4;
        } else {
            window = 11;
            degree = 2;
        }

        return savGolFilter(src, QSize(window, window), degree, degree);
    }

    BinaryThreshold OutputGenerator::adjustThreshold(BinaryThreshold threshold) const {
        const int adjusted = threshold
                             + m_colorParams.blackWhiteOptions().thresholdAdjustment();

        // Hard-bounding threshold values is necessary for example
        // if all the content went into the picture mask.
        return BinaryThreshold(qBound(30, adjusted, 225));
    }

    BinaryThreshold OutputGenerator::calcBinarizationThreshold(const QImage& image, const BinaryImage& mask) const {
        GrayscaleHistogram hist(image, mask);

        return adjustThreshold(BinaryThreshold::otsuThreshold(hist));
    }

    BinaryThreshold OutputGenerator::calcBinarizationThreshold(const QImage& image,
                                                               const QPolygonF& crop_area,
                                                               const BinaryImage* mask) const {
        QPainterPath path;
        path.addPolygon(crop_area);

        if (path.contains(image.rect())) {
            return adjustThreshold(BinaryThreshold::otsuThreshold(image));
        } else {
            BinaryImage modified_mask(image.size(), BLACK);
            PolygonRasterizer::fillExcept(modified_mask, WHITE, crop_area, Qt::WindingFill);
            modified_mask = erodeBrick(modified_mask, QSize(3, 3), WHITE);

            if (mask) {
                rasterOp<RopAnd<RopSrc, RopDst>>(modified_mask, *mask);
            }

            return calcBinarizationThreshold(image, modified_mask);
        }
    }

    BinaryImage OutputGenerator::binarize(const QImage& image) const {
        if ((image.format() == QImage::Format_Mono)
            || (image.format() == QImage::Format_MonoLSB)) {
            return BinaryImage(image);
        }

        const BlackWhiteOptions& blackWhiteOptions = m_colorParams.blackWhiteOptions();
        const BinarizationMethod binarizationMethod = blackWhiteOptions.getBinarizationMethod();

        QImage imageToBinarize = image;

        BinaryImage binarized;
        switch (binarizationMethod) {
            case OTSU: {
                GrayscaleHistogram hist(imageToBinarize);
                const BinaryThreshold bw_thresh(BinaryThreshold::otsuThreshold(hist));

                binarized = BinaryImage(imageToBinarize, adjustThreshold(bw_thresh));
                break;
            }
            case SAUVOLA: {
                QSize windowsSize = QSize(blackWhiteOptions.getWindowSize(),
                                          blackWhiteOptions.getWindowSize());
                double sauvolaCoef = blackWhiteOptions.getSauvolaCoef();

                if (m_outputProcessingParams.isWhiteOnBlackMode()) {
                    imageToBinarize = imageproc::toGrayscale(imageToBinarize);
                    imageToBinarize.invertPixels();
                }

                binarized = imageproc::binarizeSauvola(imageToBinarize, windowsSize, sauvolaCoef);

                if (m_outputProcessingParams.isWhiteOnBlackMode()) {
                    binarized.invert();
                }
                break;
            }
            case WOLF: {
                QSize windowsSize = QSize(blackWhiteOptions.getWindowSize(),
                                          blackWhiteOptions.getWindowSize());
                auto lowerBound = (unsigned char) blackWhiteOptions.getWolfLowerBound();
                auto upperBound = (unsigned char) blackWhiteOptions.getWolfUpperBound();
                double wolfCoef = blackWhiteOptions.getWolfCoef();

                if (m_outputProcessingParams.isWhiteOnBlackMode()) {
                    imageToBinarize = imageproc::toGrayscale(imageToBinarize);
                    imageToBinarize.invertPixels();
                }

                binarized = imageproc::binarizeWolf(imageToBinarize, windowsSize, lowerBound, upperBound, wolfCoef);

                if (m_outputProcessingParams.isWhiteOnBlackMode()) {
                    binarized.invert();
                }
                break;
            }
        }

        return binarized;
    }

    BinaryImage OutputGenerator::binarize(const QImage& image, const BinaryImage& mask) const {
        BinaryImage binarized = binarize(image);

        rasterOp<RopAnd<RopSrc, RopDst>>(binarized, mask);

        return binarized;
    }

    BinaryImage
    OutputGenerator::binarize(const QImage& image, const QPolygonF& crop_area, const BinaryImage* mask) const {
        QPainterPath path;
        path.addPolygon(crop_area);

        if (path.contains(image.rect()) && !mask) {
            return binarize(image);
        } else {
            BinaryImage modified_mask(image.size(), BLACK);
            PolygonRasterizer::fillExcept(modified_mask, WHITE, crop_area, Qt::WindingFill);
            modified_mask = erodeBrick(modified_mask, QSize(3, 3), WHITE);

            if (mask) {
                rasterOp<RopAnd<RopSrc, RopDst>>(modified_mask, *mask);
            }

            return binarize(image, modified_mask);
        }
    }

/**
 * \brief Remove small connected components that are considered to be garbage.
 *
 * Both the size and the distance to other components are taken into account.
 *
 * \param[in,out] image The image to despeckle.
 * \param image_rect The rectangle corresponding to \p image in the same
 *        coordinate system where m_contentRect and m_cropRect are defined.
 * \param mask_rect The area within the image to consider.  Defined not
 *        relative to \p image, but in the same coordinate system where
 *        m_contentRect and m_cropRect are defined.  This only affects
 *        \p speckles_img, if provided.
 * \param level Despeckling aggressiveness.
 * \param speckles_img If provided, the removed black speckles will be written
 *        there.  The speckles image is always considered to correspond
 *        to m_cropRect, so it will have the size of m_cropRect.size().
 *        Only the area within \p mask_rect will be copied to \p speckles_img.
 *        The rest will be filled with white.
 * \param dpi The DPI of the input image.  See the note below.
 * \param status Task status.
 * \param dbg An optional sink for debugging images.
 *
 * \note This function only works effectively when the DPI is symmetric,
 * that is, its horizontal and vertical components are equal.
 */
    void OutputGenerator::maybeDespeckleInPlace(imageproc::BinaryImage& image,
                                                const QRect& image_rect,
                                                const QRect& mask_rect,
                                                const DespeckleLevel level,
                                                BinaryImage* speckles_img,
                                                const Dpi& dpi,
                                                const TaskStatus& status,
                                                DebugImages* dbg) const {
        const QRect src_rect(mask_rect.translated(-image_rect.topLeft()));
        const QRect dst_rect(mask_rect);

        if (speckles_img) {
            BinaryImage(m_outRect.size(), WHITE).swap(*speckles_img);
            if (!mask_rect.isEmpty()) {
                rasterOp<RopSrc>(*speckles_img, dst_rect, image, src_rect.topLeft());
            }
        }

        if (level != DESPECKLE_OFF) {
            Despeckle::Level lvl = Despeckle::NORMAL;
            switch (level) {
                case DESPECKLE_CAUTIOUS:
                    lvl = Despeckle::CAUTIOUS;
                    break;
                case DESPECKLE_NORMAL:
                    lvl = Despeckle::NORMAL;
                    break;
                case DESPECKLE_AGGRESSIVE:
                    lvl = Despeckle::AGGRESSIVE;
                    break;
                default:;
            }

            Despeckle::despeckleInPlace(image, dpi, lvl, status, dbg);

            if (dbg) {
                dbg->add(image, "despeckled");
            }
        }

        if (speckles_img) {
            if (!mask_rect.isEmpty()) {
                rasterOp<RopSubtract<RopDst, RopSrc>>(
                        *speckles_img, dst_rect, image, src_rect.topLeft()
                );
            }
        }
    }  // OutputGenerator::maybeDespeckleInPlace

    void OutputGenerator::morphologicalSmoothInPlace(BinaryImage& bin_img, const TaskStatus& status) {
        // When removing black noise, remove small ones first.

        {
            const char pattern[]
                    = "XXX"
                            " - "
                            "   ";
            hitMissReplaceAllDirections(bin_img, pattern, 3, 3);
        }

        status.throwIfCancelled();

        {
            const char pattern[]
                    = "X ?"
                            "X  "
                            "X- "
                            "X- "
                            "X  "
                            "X ?";
            hitMissReplaceAllDirections(bin_img, pattern, 3, 6);
        }

        status.throwIfCancelled();

        {
            const char pattern[]
                    = "X ?"
                            "X ?"
                            "X  "
                            "X- "
                            "X- "
                            "X- "
                            "X  "
                            "X ?"
                            "X ?";
            hitMissReplaceAllDirections(bin_img, pattern, 3, 9);
        }

        status.throwIfCancelled();

        {
            const char pattern[]
                    = "XX?"
                            "XX?"
                            "XX "
                            "X+ "
                            "X+ "
                            "X+ "
                            "XX "
                            "XX?"
                            "XX?";
            hitMissReplaceAllDirections(bin_img, pattern, 3, 9);
        }

        status.throwIfCancelled();

        {
            const char pattern[]
                    = "XX?"
                            "XX "
                            "X+ "
                            "X+ "
                            "XX "
                            "XX?";
            hitMissReplaceAllDirections(bin_img, pattern, 3, 6);
        }

        status.throwIfCancelled();

        {
            const char pattern[]
                    = "   "
                            "X+X"
                            "XXX";
            hitMissReplaceAllDirections(bin_img, pattern, 3, 3);
        }
    }  // OutputGenerator::morphologicalSmoothInPlace

    void OutputGenerator::hitMissReplaceAllDirections(imageproc::BinaryImage& img,
                                                      const char* const pattern,
                                                      const int pattern_width,
                                                      const int pattern_height) {
        hitMissReplaceInPlace(img, WHITE, pattern, pattern_width, pattern_height);

        std::vector<char> pattern_data(static_cast<unsigned long long int>(pattern_width * pattern_height), ' ');
        char* const new_pattern = &pattern_data[0];

        // Rotate 90 degrees clockwise.
        const char* p = pattern;
        int new_width = pattern_height;
        int new_height = pattern_width;
        for (int y = 0; y < pattern_height; ++y) {
            for (int x = 0; x < pattern_width; ++x, ++p) {
                const int new_x = pattern_height - 1 - y;
                const int new_y = x;
                new_pattern[new_y * new_width + new_x] = *p;
            }
        }
        hitMissReplaceInPlace(img, WHITE, new_pattern, new_width, new_height);

        // Rotate upside down.
        p = pattern;
        new_width = pattern_width;
        new_height = pattern_height;
        for (int y = 0; y < pattern_height; ++y) {
            for (int x = 0; x < pattern_width; ++x, ++p) {
                const int new_x = pattern_width - 1 - x;
                const int new_y = pattern_height - 1 - y;
                new_pattern[new_y * new_width + new_x] = *p;
            }
        }
        hitMissReplaceInPlace(img, WHITE, new_pattern, new_width, new_height);
        // Rotate 90 degrees counter-clockwise.
        p = pattern;
        new_width = pattern_height;
        new_height = pattern_width;
        for (int y = 0; y < pattern_height; ++y) {
            for (int x = 0; x < pattern_width; ++x, ++p) {
                const int new_x = y;
                const int new_y = pattern_width - 1 - x;
                new_pattern[new_y * new_width + new_x] = *p;
            }
        }
        hitMissReplaceInPlace(img, WHITE, new_pattern, new_width, new_height);
    }  // OutputGenerator::hitMissReplaceAllDirections

    QSize OutputGenerator::calcLocalWindowSize(const Dpi& dpi) {
        const QSizeF size_mm(3, 30);
        const QSizeF size_inch(size_mm * constants::MM2INCH);
        const QSizeF size_pixels_f(
                dpi.horizontal() * size_inch.width(),
                dpi.vertical() * size_inch.height()
        );
        QSize size_pixels(size_pixels_f.toSize());

        if (size_pixels.width() < 3) {
            size_pixels.setWidth(3);
        }
        if (size_pixels.height() < 3) {
            size_pixels.setHeight(3);
        }

        return size_pixels;
    }

    void OutputGenerator::applyFillZonesInPlace(QImage& img,
                                                const ZoneSet& zones,
                                                boost::function<QPointF(const QPointF&)>const & orig_to_output,
                                                const QTransform& postTransform)
    const {
        if (zones.empty()) {
            return;
        }

        QImage canvas(img.convertToFormat(QImage::Format_ARGB32_Premultiplied));

        {
            QPainter painter(&canvas);
            painter.setRenderHint(QPainter::Antialiasing, true);
            painter.setPen(Qt::NoPen);

            for (const Zone& zone : zones) {
                const QColor color(zone.properties().locateOrDefault<FillColorProperty>()->color());
                const QPolygonF poly(postTransform.map(zone.spline().transformed(orig_to_output).toPolygon()));
                painter.setBrush(color);
                painter.drawPolygon(poly, Qt::WindingFill);
            }
        }

        if ((img.format() == QImage::Format_Indexed8) && img.isGrayscale()) {
            img = toGrayscale(canvas);
        } else {
            img = canvas.convertToFormat(img.format());
        }
    }

    void OutputGenerator::applyFillZonesInPlace(QImage& img,
                                                const ZoneSet& zones,
                                                boost::function<QPointF(const QPointF&)>const & orig_to_output) const {
        applyFillZonesInPlace(
                img, zones, orig_to_output, QTransform()
        );
    }

    void OutputGenerator::applyFillZonesInPlace(QImage& img,
                                                const ZoneSet& zones,
                                                const QTransform& postTransform) const {
        typedef QPointF (QTransform::* MapPointFunc)(const QPointF&) const;
        applyFillZonesInPlace(
                img, zones, boost::bind((MapPointFunc) &QTransform::map, m_xform.transform(), _1), postTransform
        );
    }

/**
 * A simplified version of the above, using toOutput() for translation
 * from original image to output image coordinates.
 */
    void OutputGenerator::applyFillZonesInPlace(QImage& img, const ZoneSet& zones) const {
        typedef QPointF (QTransform::* MapPointFunc)(const QPointF&) const;
        applyFillZonesInPlace(
                img, zones, boost::bind((MapPointFunc) &QTransform::map, m_xform.transform(), _1)
        );
    }

    void
    OutputGenerator::applyFillZonesInPlace(imageproc::BinaryImage& img,
                                           const ZoneSet& zones,
                                           boost::function<QPointF(const QPointF&)>const & orig_to_output,
                                           const QTransform& postTransform) const {
        if (zones.empty()) {
            return;
        }

        for (const Zone& zone : zones) {
            const QColor color(zone.properties().locateOrDefault<FillColorProperty>()->color());
            const BWColor bw_color = qGray(color.rgb()) < 128 ? BLACK : WHITE;
            const QPolygonF poly(postTransform.map(zone.spline().transformed(orig_to_output).toPolygon()));
            PolygonRasterizer::fill(img, bw_color, poly, Qt::WindingFill);
        }
    }

    void
    OutputGenerator::applyFillZonesInPlace(imageproc::BinaryImage& img,
                                           const ZoneSet& zones,
                                           boost::function<QPointF(const QPointF&)>const & orig_to_output) const {
        applyFillZonesInPlace(
                img, zones, orig_to_output, QTransform()
        );
    }

    void OutputGenerator::applyFillZonesInPlace(imageproc::BinaryImage& img,
                                                const ZoneSet& zones,
                                                const QTransform& postTransform) const {
        typedef QPointF (QTransform::* MapPointFunc)(const QPointF&) const;
        applyFillZonesInPlace(
                img, zones, boost::bind((MapPointFunc) &QTransform::map, m_xform.transform(), _1), postTransform
        );
    }

/**
 * A simplified version of the above, using toOutput() for translation
 * from original image to output image coordinates.
 */
    void OutputGenerator::applyFillZonesInPlace(imageproc::BinaryImage& img, const ZoneSet& zones) const {
        typedef QPointF (QTransform::* MapPointFunc)(const QPointF&) const;
        applyFillZonesInPlace(
                img, zones, boost::bind((MapPointFunc) &QTransform::map, m_xform.transform(), _1)
        );
    }

    void OutputGenerator::movePointToTopMargin(BinaryImage& bw_image, XSpline& spline, int idx) const {
        QPointF pos = spline.controlPointPosition(idx);

        for (int j = 0; j < pos.y(); j++) {
            if (bw_image.getPixel(static_cast<int>(pos.x()), j) == WHITE) {
                int count = 0;
                int check_num = 16;

                for (int jj = j; jj < (j + check_num); jj++) {
                    if (bw_image.getPixel(static_cast<int>(pos.x()), jj) == WHITE) {
                        count++;
                    }
                }

                if (count == check_num) {
                    pos.setY(j);

                    spline.moveControlPoint(idx, pos);

                    break;
                }
            }
        }
    }

    void OutputGenerator::movePointToBottomMargin(BinaryImage& bw_image, XSpline& spline, int idx) const {
        QPointF pos = spline.controlPointPosition(idx);

        for (int j = bw_image.height() - 1; j > pos.y(); j--) {
            if (bw_image.getPixel(static_cast<int>(pos.x()), j) == WHITE) {
                int count = 0;
                int check_num = 16;

                for (int jj = j; jj > (j - check_num); jj--) {
                    if (bw_image.getPixel(static_cast<int>(pos.x()), jj) == WHITE) {
                        count++;
                    }
                }

                if (count == check_num) {
                    pos.setY(j);

                    spline.moveControlPoint(idx, pos);

                    break;
                }
            }
        }
    }

    void OutputGenerator::drawPoint(QImage& image, const QPointF& pt) const {
        QPoint pts = pt.toPoint();

        for (int i = pts.x() - 10; i < pts.x() + 10; i++) {
            for (int j = pts.y() - 10; j < pts.y() + 10; j++) {
                QPoint p1(i, j);

                image.setPixel(p1, qRgb(255, 0, 0));
            }
        }
    }

    void OutputGenerator::movePointToTopMargin(BinaryImage& bw_image, std::vector<QPointF>& polyline, int idx) const {
        QPointF& pos = polyline[idx];

        for (int j = 0; j < pos.y(); j++) {
            if (bw_image.getPixel(static_cast<int>(pos.x()), j) == WHITE) {
                int count = 0;
                int check_num = 16;

                for (int jj = j; jj < (j + check_num); jj++) {
                    if (bw_image.getPixel(static_cast<int>(pos.x()), jj) == WHITE) {
                        count++;
                    }
                }

                if (count == check_num) {
                    pos.setY(j);

                    break;
                }
            }
        }
    }

    void
    OutputGenerator::movePointToBottomMargin(BinaryImage& bw_image, std::vector<QPointF>& polyline, int idx) const {
        QPointF& pos = polyline[idx];

        for (int j = bw_image.height() - 1; j > pos.y(); j--) {
            if (bw_image.getPixel(static_cast<int>(pos.x()), j) == WHITE) {
                int count = 0;
                int check_num = 16;

                for (int jj = j; jj > (j - check_num); jj--) {
                    if (bw_image.getPixel(static_cast<int>(pos.x()), jj) == WHITE) {
                        count++;
                    }
                }

                if (count == check_num) {
                    pos.setY(j);

                    break;
                }
            }
        }
    }

    float OutputGenerator::vert_border_skew_angle(const QPointF& top, const QPointF& bottom) const {
        return static_cast<float>(qFabs(qAtan((bottom.x() - top.x()) / (bottom.y() - top.y())) * 180 / M_PI));
    }

    void OutputGenerator::deskew(QImage* image, const double angle, const QColor& outside_color) const {
        if (angle == .0) {
            return;
        }

        QPointF center(image->width() / 2, image->height() / 2);

        QTransform rot;
        rot.translate(center.x(), center.y());
        rot.rotate(-angle);
        rot.translate(-center.x(), -center.y());

        *image = imageproc::transform(*image, rot, image->rect(),
                                      OutsidePixels::assumeWeakColor(outside_color));
    }

    double OutputGenerator::maybe_deskew(QImage* p_dewarped,
                                         DewarpingOptions m_dewarpingOptions,
                                         const QColor& outside_color) const {
        if (m_dewarpingOptions.needPostDeskew()
            && ((m_dewarpingOptions.dewarpingMode() == MARGINAL)
                || (m_dewarpingOptions.dewarpingMode() == MANUAL))) {
            BinaryThreshold bw_threshold(128);
            BinaryImage bw_image(*p_dewarped, bw_threshold);

            SkewFinder skew_finder;
            const Skew skew(skew_finder.findSkew(bw_image));
            if ((skew.angle() != 0.0) && (skew.confidence() >= Skew::GOOD_CONFIDENCE)) {
                const double angle_deg = skew.angle();

                deskew(p_dewarped, angle_deg, outside_color);

                return angle_deg;
            }
        }

        return .0;
    }

    const QTransform& OutputGenerator::getPostTransform() const {
        return postTransform;
    }

    void OutputGenerator::applyFillZonesToMixedInPlace(QImage& img,
                                                       const ZoneSet& zones,
                                                       const BinaryImage& picture_mask) const {
        BinaryImage bw_content(img, BinaryThreshold(1));
        applyFillZonesInPlace(bw_content, zones);
        applyFillZonesInPlace(img, zones);
        combineMixed(img, bw_content, picture_mask, false);
    }

    void OutputGenerator::applyFillZonesToMixedInPlace(QImage& img,
                                                       const ZoneSet& zones,
                                                       boost::function<QPointF(const QPointF&)>const & orig_to_output,
                                                       const QTransform& postTransform,
                                                       const BinaryImage& picture_mask) const {
        BinaryImage bw_content(img, BinaryThreshold(1));
        applyFillZonesInPlace(bw_content, zones, orig_to_output, postTransform);
        applyFillZonesInPlace(img, zones, orig_to_output, postTransform);
        combineMixed(img, bw_content, picture_mask, false);
    }
}  // namespace output