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
#include <AdjustBrightness.h>
#include <Binarize.h>
#include <BlackOnWhiteEstimator.h>
#include <ConnCompEraser.h>
#include <ConnectivityMap.h>
#include <Constants.h>
#include <CylindricalSurfaceDewarper.h>
#include <Despeckle.h>
#include <DewarpingPointMapper.h>
#include <DistortionModelBuilder.h>
#include <DrawOver.h>
#include <GrayRasterOp.h>
#include <Grayscale.h>
#include <InfluenceMap.h>
#include <Morphology.h>
#include <OrthogonalRotation.h>
#include <PolygonRasterizer.h>
#include <PolynomialSurface.h>
#include <RasterDewarper.h>
#include <RasterOp.h>
#include <SavGolFilter.h>
#include <Scale.h>
#include <SeedFill.h>
#include <TextLineTracer.h>
#include <TopBottomEdgeTracer.h>
#include <Transform.h>
#include <imageproc/BackgroundColorCalculator.h>
#include <imageproc/ColorSegmenter.h>
#include <imageproc/ImageCombination.h>
#include <imageproc/OrthogonalRotation.h>
#include <imageproc/PolygonUtils.h>
#include <imageproc/Posterizer.h>
#include <imageproc/SkewFinder.h>
#include <QColor>
#include <QDebug>
#include <QPainter>
#include <QPointF>
#include <QPolygonF>
#include <QSize>
#include <QTransform>
#include <QtCore/QSettings>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <cmath>
#include "ColorParams.h"
#include "DebugImages.h"
#include "DewarpingOptions.h"
#include "Dpm.h"
#include "EstimateBackground.h"
#include "FillColorProperty.h"
#include "FilterData.h"
#include "ForegroundType.h"
#include "OutputImageBuilder.h"
#include "OutputProcessingParams.h"
#include "Params.h"
#include "PictureLayerProperty.h"
#include "PictureShapeOptions.h"
#include "RenderParams.h"
#include "Settings.h"
#include "TaskStatus.h"
#include "Utils.h"
#include "ZoneCategoryProperty.h"

using namespace imageproc;
using namespace dewarping;

namespace output {
OutputGenerator::OutputGenerator(const ImageTransformation& xform, const QPolygonF& content_rect_phys)
    : m_xform(xform),
      m_outRect(xform.resultingRect().toRect()),
      m_contentRect(xform.transform().map(content_rect_phys).boundingRect().toRect()) {
  assert(m_outRect.topLeft() == QPoint(0, 0));

  if (!m_contentRect.isEmpty()) {
    // prevents a crash due to round error on transforming virtual coordinates to output image coordinates
    // when m_contentRect coordinates could exceed m_outRect ones by 1 px
    m_contentRect = m_contentRect.intersected(m_outRect);
    // Note that QRect::contains(<empty rect>) always returns false, so we don't use it here.
    assert(m_outRect.contains(m_contentRect.topLeft()) && m_outRect.contains(m_contentRect.bottomRight()));
  }
}

QSize OutputGenerator::outputImageSize() const {
  return m_outRect.size();
}

QRect OutputGenerator::outputContentRect() const {
  return m_contentRect;
}

class OutputGenerator::Processor {
 public:
  Processor(const OutputGenerator& generator,
            const PageId& pageId,
            const intrusive_ptr<Settings>& settings,
            const FilterData& input,
            const TaskStatus& status,
            DebugImages* dbg);

  std::unique_ptr<OutputImage> process(ZoneSet& picture_zones,
                                       const ZoneSet& fill_zones,
                                       DistortionModel& distortion_model,
                                       const DepthPerception& depth_perception,
                                       BinaryImage* auto_picture_mask,
                                       BinaryImage* speckles_image);

 private:
  void initParams();

  void calcAreas();

  void updateBlackOnWhite(const FilterData& input);

  void initFilterData(const FilterData& input);

  std::unique_ptr<OutputImage> processImpl(ZoneSet& picture_zones,
                                           const ZoneSet& fill_zones,
                                           DistortionModel& distortion_model,
                                           const DepthPerception& depth_perception,
                                           BinaryImage* auto_picture_mask,
                                           BinaryImage* speckles_image);

  std::unique_ptr<OutputImage> processWithoutDewarping(ZoneSet& picture_zones,
                                                       const ZoneSet& fill_zones,
                                                       BinaryImage* auto_picture_mask,
                                                       BinaryImage* speckles_image);

  std::unique_ptr<OutputImage> processWithDewarping(ZoneSet& picture_zones,
                                                    const ZoneSet& fill_zones,
                                                    DistortionModel& distortion_model,
                                                    const DepthPerception& depth_perception,
                                                    BinaryImage* auto_picture_mask,
                                                    BinaryImage* speckles_image);

  std::unique_ptr<OutputImage> buildEmptyImage() const;

  double findSkew(const QImage& image) const;

  void setupTrivialDistortionModel(DistortionModel& distortion_model) const;

  static CylindricalSurfaceDewarper createDewarper(const DistortionModel& distortion_model,
                                                   const QTransform& distortion_model_to_target,
                                                   double depth_perception);

  QImage dewarp(const QTransform& orig_to_src,
                const QImage& src,
                const QTransform& src_to_output,
                const DistortionModel& distortion_model,
                const DepthPerception& depth_perception,
                const QColor& bg_color) const;

  GrayImage normalizeIlluminationGray(const QImage& input,
                                      const QPolygonF& area_to_consider,
                                      const QTransform& xform,
                                      const QRect& target_rect,
                                      GrayImage* background = nullptr) const;

  GrayImage detectPictures(const GrayImage& input_300dpi) const;

  BinaryImage estimateBinarizationMask(const GrayImage& gray_source,
                                       const QRect& source_rect,
                                       const QRect& source_sub_rect) const;

  void modifyBinarizationMask(BinaryImage& bw_mask, const QRect& mask_rect, const ZoneSet& zones) const;

  BinaryThreshold adjustThreshold(BinaryThreshold threshold) const;

  BinaryThreshold calcBinarizationThreshold(const QImage& image, const BinaryImage& mask) const;

  BinaryThreshold calcBinarizationThreshold(const QImage& image,
                                            const QPolygonF& crop_area,
                                            const BinaryImage* mask = nullptr) const;

  void morphologicalSmoothInPlace(BinaryImage& bin_img) const;

  BinaryImage binarize(const QImage& image) const;

  BinaryImage binarize(const QImage& image, const BinaryImage& mask) const;

  BinaryImage binarize(const QImage& image, const QPolygonF& crop_area, const BinaryImage* mask = nullptr) const;

  void maybeDespeckleInPlace(BinaryImage& image,
                             const QRect& image_rect,
                             const QRect& mask_rect,
                             double level,
                             BinaryImage* speckles_img,
                             const Dpi& dpi) const;

  QImage segmentImage(const BinaryImage& image, const QImage& color_image, const BinaryImage* mask = nullptr) const;

  QImage posterizeImage(const QImage& image, const QColor& background_color = Qt::white) const;

  ForegroundType getForegroundType() const;

  void processPictureZones(BinaryImage& mask, ZoneSet& picture_zones, const GrayImage& image);

  QImage transformToWorkingCs(bool normalize) const;

  DistortionModel buildAutoDistortionModel(const GrayImage& warped_gray_output, const QTransform& to_original) const;

  DistortionModel buildMarginalDistortionModel() const;

  const ImageTransformation& m_xform;
  const QRect& m_outRect;
  const QRect& m_contentRect;

  const PageId m_pageId;
  const intrusive_ptr<Settings> m_settings;

  Dpi m_dpi;
  ColorParams m_colorParams;
  SplittingOptions m_splittingOptions;
  PictureShapeOptions m_pictureShapeOptions;
  DewarpingOptions m_dewarpingOptions;
  double m_despeckleLevel;
  OutputProcessingParams m_outputProcessingParams;

  RenderParams m_renderParams;

  QPolygonF m_preCropArea;
  QPolygonF m_croppedContentArea;
  QRect m_croppedContentRect;
  QPolygonF m_outCropArea;
  QSize m_targetSize;
  QRect m_workingBoundingRect;
  QRect m_contentRectInWorkingCs;
  QPolygonF m_contentAreaInWorkingCs;
  QPolygonF m_outCropAreaInWorkingCs;
  QPolygonF m_preCropAreaInOriginalCs;
  QPolygonF m_contentAreaInOriginalCs;
  QPolygonF m_outCropAreaInOriginalCs;
  bool m_blank;

  bool m_blackOnWhite;
  QImage m_inputOrigImage;
  GrayImage m_inputGrayImage;
  bool m_colorOriginal;

  const TaskStatus& m_status;
  DebugImages* const m_dbg;

  QColor m_outsideBackgroundColor;
};

std::unique_ptr<OutputImage> OutputGenerator::process(const TaskStatus& status,
                                                      const FilterData& input,
                                                      ZoneSet& picture_zones,
                                                      const ZoneSet& fill_zones,
                                                      DistortionModel& distortion_model,
                                                      const DepthPerception& depth_perception,
                                                      BinaryImage* auto_picture_mask,
                                                      BinaryImage* speckles_image,
                                                      DebugImages* dbg,
                                                      const PageId& pageId,
                                                      const intrusive_ptr<Settings>& settings) const {
  return Processor(*this, pageId, settings, input, status, dbg)
      .process(picture_zones, fill_zones, distortion_model, depth_perception, auto_picture_mask, speckles_image);
}

OutputGenerator::Processor::Processor(const OutputGenerator& generator,
                                      const PageId& pageId,
                                      const intrusive_ptr<Settings>& settings,
                                      const FilterData& input,
                                      const TaskStatus& status,
                                      DebugImages* dbg)
    : m_xform(generator.m_xform),
      m_outRect(generator.m_outRect),
      m_contentRect(generator.m_contentRect),
      m_pageId(pageId),
      m_settings(settings),
      m_despeckleLevel(0),
      m_blank(false),
      m_blackOnWhite(true),
      m_colorOriginal(false),
      m_status(status),
      m_dbg(dbg) {
  initParams();
  calcAreas();
  if (!m_blank) {
    initFilterData(input);
  }
}

std::unique_ptr<OutputImage> OutputGenerator::Processor::process(ZoneSet& picture_zones,
                                                                 const ZoneSet& fill_zones,
                                                                 DistortionModel& distortion_model,
                                                                 const DepthPerception& depth_perception,
                                                                 BinaryImage* auto_picture_mask,
                                                                 BinaryImage* speckles_image) {
  std::unique_ptr<OutputImage> image
      = processImpl(picture_zones, fill_zones, distortion_model, depth_perception, auto_picture_mask, speckles_image);
  image->setDpm(m_dpi);
  return image;
}

void OutputGenerator::Processor::initParams() {
  const Params params = m_settings->getParams(m_pageId);
  m_dpi = params.outputDpi();
  m_colorParams = params.colorParams();
  m_splittingOptions = params.splittingOptions();
  m_pictureShapeOptions = params.pictureShapeOptions();
  m_dewarpingOptions = params.dewarpingOptions();
  m_despeckleLevel = params.despeckleLevel();

  m_outputProcessingParams = m_settings->getOutputProcessingParams(m_pageId);

  m_renderParams = RenderParams(m_colorParams, m_splittingOptions);
}

void OutputGenerator::Processor::calcAreas() {
  m_targetSize = m_outRect.size().expandedTo(QSize(1, 1));

  if (m_renderParams.fillOffcut()) {
    m_preCropArea = m_xform.resultingPreCropArea();
  } else {
    const QPolygonF imageRectInOutputCs = m_xform.transform().map(m_xform.origRect());
    m_preCropArea = imageRectInOutputCs.intersected(QRectF(m_outRect));
  }

  m_croppedContentArea = m_preCropArea.intersected(QRectF(m_renderParams.fillMargins() ? m_contentRect : m_outRect));
  m_croppedContentRect = m_croppedContentArea.boundingRect().toRect();
  m_croppedContentArea = m_croppedContentArea.intersected(QRectF(m_croppedContentRect));
  if (m_croppedContentRect.isEmpty()) {
    m_blank = true;
    return;
  }

  m_outCropArea = m_preCropArea.intersected(QRectF(m_outRect));

  // For various reasons, we need some whitespace around the content
  // area.  This is the number of pixels of such whitespace.
  const int content_margin = m_dpi.vertical() * 20 / 300;
  // The content area (in output image coordinates) extended
  // with content_margin.  Note that we prevent that extension
  // from reaching the neighboring page.
  // This is the area we are going to pass to estimateBackground().
  // estimateBackground() needs some margins around content, and
  // generally smaller margins are better, except when there is
  // some garbage that connects the content to the edge of the
  // image area.
  m_workingBoundingRect = m_preCropArea
                              .intersected(QRectF(m_croppedContentRect.adjusted(-content_margin, -content_margin,
                                                                                content_margin, content_margin)))
                              .boundingRect()
                              .toRect();
  m_contentRectInWorkingCs = m_croppedContentRect.translated(-m_workingBoundingRect.topLeft());
  m_contentAreaInWorkingCs = m_croppedContentArea.translated(-m_workingBoundingRect.topLeft());
  m_outCropAreaInWorkingCs = m_outCropArea.translated(-m_workingBoundingRect.topLeft());
  m_preCropAreaInOriginalCs = m_xform.transformBack().map(m_preCropArea);
  m_contentAreaInOriginalCs = m_xform.transformBack().map(m_croppedContentArea);
  m_outCropAreaInOriginalCs = m_xform.transformBack().map(m_outCropArea);
}

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


template <typename PixelType>
PixelType reserveBlackAndWhite(PixelType color);

template <>
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

template <>
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

template <typename PixelType>
void reserveBlackAndWhite(QImage& img) {
  const int width = img.width();
  const int height = img.height();

  auto* image_line = reinterpret_cast<PixelType*>(img.bits());
  const int image_stride = img.bytesPerLine() / sizeof(PixelType);

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      image_line[x] = reserveBlackAndWhite<PixelType>(image_line[x]);
    }
    image_line += image_stride;
  }
}

void reserveBlackAndWhite(QImage& img) {
  switch (img.format()) {
    case QImage::Format_Indexed8:
      reserveBlackAndWhite<uint8_t>(img);
      break;
    case QImage::Format_RGB32:
    case QImage::Format_ARGB32:
      reserveBlackAndWhite<uint32_t>(img);
      break;
    default:
      throw std::invalid_argument("reserveBlackAndWhite: wrong image format.");
  }
}

template <typename PixelType>
void reserveBlackAndWhite(QImage& img, const BinaryImage& mask) {
  const int width = img.width();
  const int height = img.height();

  auto* image_line = reinterpret_cast<PixelType*>(img.bits());
  const int image_stride = img.bytesPerLine() / sizeof(PixelType);
  const uint32_t* mask_line = mask.data();
  const int mask_stride = mask.wordsPerLine();
  const uint32_t msb = uint32_t(1) << 31;

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      if (mask_line[x >> 5] & (msb >> (x & 31))) {
        image_line[x] = reserveBlackAndWhite<PixelType>(image_line[x]);
      }
    }
    image_line += image_stride;
    mask_line += mask_stride;
  }
}

void reserveBlackAndWhite(QImage& img, const BinaryImage& mask) {
  switch (img.format()) {
    case QImage::Format_Indexed8:
      reserveBlackAndWhite<uint8_t>(img, mask);
      break;
    case QImage::Format_RGB32:
    case QImage::Format_ARGB32:
      reserveBlackAndWhite<uint32_t>(img, mask);
      break;
    default:
      throw std::invalid_argument("reserveBlackAndWhite: wrong image format.");
  }
}

template <typename MixedPixel>
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

void fillMarginsInPlace(QImage& image,
                        const QPolygonF& content_poly,
                        const QColor& color,
                        const bool antialiasing = true) {
  if (content_poly.intersected(QRectF(image.rect())) != content_poly) {
    throw std::invalid_argument("fillMarginsInPlace: the content area exceeds image rect.");
  }

  if ((image.format() == QImage::Format_Mono) || (image.format() == QImage::Format_MonoLSB)) {
    BinaryImage binaryImage(image);
    PolygonRasterizer::fillExcept(binaryImage, (color == Qt::black) ? BLACK : WHITE, content_poly, Qt::WindingFill);
    image = binaryImage.toQImage();
    return;
  }
  if ((image.format() == QImage::Format_Indexed8) && image.isGrayscale()) {
    PolygonRasterizer::grayFillExcept(image, static_cast<unsigned char>(qGray(color.rgb())), content_poly,
                                      Qt::WindingFill);
    return;
  }

  assert(image.format() == QImage::Format_RGB32 || image.format() == QImage::Format_ARGB32);

  const QImage::Format imageFormat = image.format();
  image = image.convertToFormat(QImage::Format_ARGB32_Premultiplied);
  {
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing, antialiasing);
    painter.setBrush(color);
    painter.setPen(Qt::NoPen);

    QPainterPath outer_path;
    outer_path.addRect(image.rect());
    QPainterPath inner_path;
    inner_path.addPolygon(PolygonUtils::round(content_poly));

    painter.drawPath(outer_path.subtracted(inner_path));
  }
  image = image.convertToFormat(imageFormat);
}

void fillMarginsInPlace(BinaryImage& image, const QPolygonF& content_poly, const BWColor color) {
  if (content_poly.intersected(QRectF(image.rect())) != content_poly) {
    throw std::invalid_argument("fillMarginsInPlace: the content area exceeds image rect.");
  }

  PolygonRasterizer::fillExcept(image, color, content_poly, Qt::WindingFill);
}

void fillMarginsInPlace(BinaryImage& image, const BinaryImage& content_mask, const BWColor color) {
  if (image.size() != content_mask.size()) {
    throw std::invalid_argument("fillMarginsInPlace: img and mask have different sizes");
  }

  fillExcept(image, content_mask, color);
}

void fillMarginsInPlace(QImage& image, const BinaryImage& content_mask, const QColor& color) {
  if (image.size() != content_mask.size()) {
    throw std::invalid_argument("fillMarginsInPlace: img and mask have different sizes");
  }

  if ((image.format() == QImage::Format_Mono) || (image.format() == QImage::Format_MonoLSB)) {
    BinaryImage binaryImage(image);
    fillExcept(binaryImage, content_mask, (color == Qt::black) ? BLACK : WHITE);
    image = binaryImage.toQImage();
    return;
  }

  if ((image.format() == QImage::Format_Indexed8) && image.isGrayscale()) {
    fillExcept<uint8_t>(image, content_mask, color);
  } else {
    assert(image.format() == QImage::Format_RGB32 || image.format() == QImage::Format_ARGB32);
    fillExcept<uint32_t>(image, content_mask, color);
  }
}

void removeAutoPictureZones(ZoneSet& picture_zones) {
  for (auto it = picture_zones.begin(); it != picture_zones.end();) {
    const Zone& zone = *it;
    if (zone.properties().locateOrDefault<ZoneCategoryProperty>()->zone_category() == ZoneCategoryProperty::AUTO) {
      it = picture_zones.erase(it);
    } else {
      ++it;
    }
  }
}

Zone createPictureZoneFromPoly(const QPolygonF& polygon) {
  PropertySet propertySet;
  propertySet.locateOrCreate<output::PictureLayerProperty>()->setLayer(output::PictureLayerProperty::PAINTER2);
  propertySet.locateOrCreate<output::ZoneCategoryProperty>()->setZoneCategory(ZoneCategoryProperty::AUTO);
  return Zone(SerializableSpline(polygon), propertySet);
}

void applyFillZonesInPlace(QImage& img,
                           const ZoneSet& zones,
                           const boost::function<QPointF(const QPointF&)>& orig_to_output,
                           bool antialiasing = true) {
  if (zones.empty()) {
    return;
  }

  QImage canvas(img.convertToFormat(QImage::Format_ARGB32_Premultiplied));
  {
    QPainter painter(&canvas);
    painter.setRenderHint(QPainter::Antialiasing, antialiasing);
    painter.setPen(Qt::NoPen);

    for (const Zone& zone : zones) {
      const QColor color(zone.properties().locateOrDefault<FillColorProperty>()->color());
      const QPolygonF poly(zone.spline().transformed(orig_to_output).toPolygon());
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

using MapPointFunc = QPointF (QTransform::*)(const QPointF&) const;

void applyFillZonesInPlace(QImage& img, const ZoneSet& zones, const QTransform& transform, bool antialiasing = true) {
  applyFillZonesInPlace(img, zones, boost::bind(static_cast<MapPointFunc>(&QTransform::map), transform, _1),
                        antialiasing);
}

void applyFillZonesInPlace(BinaryImage& img,
                           const ZoneSet& zones,
                           const boost::function<QPointF(const QPointF&)>& orig_to_output) {
  if (zones.empty()) {
    return;
  }

  for (const Zone& zone : zones) {
    const QColor color(zone.properties().locateOrDefault<FillColorProperty>()->color());
    const BWColor bw_color = qGray(color.rgb()) < 128 ? BLACK : WHITE;
    const QPolygonF poly(zone.spline().transformed(orig_to_output).toPolygon());
    PolygonRasterizer::fill(img, bw_color, poly, Qt::WindingFill);
  }
}

void applyFillZonesInPlace(BinaryImage& img, const ZoneSet& zones, const QTransform& transform) {
  applyFillZonesInPlace(img, zones, boost::bind(static_cast<MapPointFunc>(&QTransform::map), transform, _1));
}

void applyFillZonesToMixedInPlace(QImage& img,
                                  const ZoneSet& zones,
                                  const boost::function<QPointF(const QPointF&)>& orig_to_output,
                                  const BinaryImage& picture_mask,
                                  bool binary_mode) {
  if (binary_mode) {
    BinaryImage bw_content(img, BinaryThreshold(1));
    applyFillZonesInPlace(bw_content, zones, orig_to_output);
    applyFillZonesInPlace(img, zones, orig_to_output);
    combineImages(img, bw_content, picture_mask);
  } else {
    QImage content(img);
    applyMask(content, picture_mask);
    applyFillZonesInPlace(content, zones, orig_to_output, false);
    applyFillZonesInPlace(img, zones, orig_to_output);
    combineImages(img, content, picture_mask);
  }
}

void applyFillZonesToMixedInPlace(QImage& img,
                                  const ZoneSet& zones,
                                  const QTransform& transform,
                                  const BinaryImage& picture_mask,
                                  bool binary_mode) {
  applyFillZonesToMixedInPlace(img, zones, boost::bind(static_cast<MapPointFunc>(&QTransform::map), transform, _1),
                               picture_mask, binary_mode);
}

void applyFillZonesToMask(BinaryImage& mask,
                          const ZoneSet& zones,
                          const boost::function<QPointF(const QPointF&)>& orig_to_output,
                          const BWColor fill_color = BLACK) {
  if (zones.empty()) {
    return;
  }

  for (const Zone& zone : zones) {
    const QPolygonF poly(zone.spline().transformed(orig_to_output).toPolygon());
    PolygonRasterizer::fill(mask, fill_color, poly, Qt::WindingFill);
  }
}

void applyFillZonesToMask(BinaryImage& mask,
                          const ZoneSet& zones,
                          const QTransform& transform,
                          const BWColor fill_color = BLACK) {
  applyFillZonesToMask(mask, zones, boost::bind((MapPointFunc) &QTransform::map, transform, _1), fill_color);
}

const int MultiplyDeBruijnBitPosition[32] = {0,  1,  28, 2,  29, 14, 24, 3, 30, 22, 20, 15, 25, 17, 4,  8,
                                             31, 27, 13, 23, 21, 19, 16, 7, 26, 12, 18, 6,  11, 5,  10, 9};

const int MultiplyDeBruijnBitPosition2[32] = {0, 9,  1,  10, 13, 21, 2,  29, 11, 14, 16, 18, 22, 25, 3, 30,
                                              8, 12, 20, 28, 15, 17, 24, 7,  19, 27, 23, 6,  26, 5,  4, 31};

/**
 * aka "Count the consecutive zero bits (trailing) on the right with multiply and lookup"
 * from Bit Twiddling Hacks By Sean Eron Anderson
 *
 * https://graphics.stanford.edu/~seander/bithacks.html#ZerosOnRightMultLookup
 */
inline int countConsecutiveZeroBitsTrailing(uint32_t v) {
  return MultiplyDeBruijnBitPosition[((uint32_t)((v & -signed(v)) * 0x077CB531U)) >> 27];
}

/**
 * aka "Find the log base 2 of an N-bit integer in O(lg(N)) operations with multiply and lookup"
 * from Bit Twiddling Hacks By Sean Eron Anderson
 *
 * https://graphics.stanford.edu/~seander/bithacks.html#IntegerLogDeBruijn
 */
inline int findPositionOfTheHighestBitSet(uint32_t v) {
  v |= v >> 1;  // first round down to one less than a power of 2
  v |= v >> 2;
  v |= v >> 4;
  v |= v >> 8;
  v |= v >> 16;

  return MultiplyDeBruijnBitPosition2[(uint32_t)(v * 0x07C4ACDDU) >> 27];
}

std::vector<QRect> findRectAreas(const BinaryImage& mask, BWColor content_color, int sensitivity) {
  if (mask.isNull()) {
    return {};
  }

  std::vector<QRect> areas;

  const int w = mask.width();
  const int h = mask.height();
  const int wpl = mask.wordsPerLine();
  const int last_word_idx = (w - 1) >> 5;
  const int last_word_bits = w - (last_word_idx << 5);
  const int last_word_unused_bits = 32 - last_word_bits;
  const uint32_t last_word_mask = ~uint32_t(0) << last_word_unused_bits;
  const uint32_t modifier = (content_color == WHITE) ? ~uint32_t(0) : 0;
  const uint32_t* const data = mask.data();

  const uint32_t* line = data;
  // create list of filled continuous blocks on each line
  for (int y = 0; y < h; ++y, line += wpl) {
    QRect area;
    area.setTop(y);
    area.setBottom(y);
    bool area_found = false;
    for (int i = 0; i <= last_word_idx; ++i) {
      uint32_t word = line[i] ^ modifier;
      if (i == last_word_idx) {
        // The last (possibly incomplete) word.
        word &= last_word_mask;
      }
      if (word) {
        if (!area_found) {
          area.setLeft((i << 5) + 31 - findPositionOfTheHighestBitSet(~line[i]));
          area_found = true;
        }
        area.setRight(((i + 1) << 5) - 1);
      } else {
        if (area_found) {
          uint32_t v = line[i - 1];
          if (v) {
            area.setRight(area.right() - countConsecutiveZeroBitsTrailing(~v));
          }
          areas.emplace_back(area);
          area_found = false;
        }
      }
    }
    if (area_found) {
      uint32_t v = line[last_word_idx];
      if (v) {
        area.setRight(area.right() - countConsecutiveZeroBitsTrailing(~v));
      }
      areas.emplace_back(area);
    }
  }

  // join adjacent blocks of areas
  bool join = true;
  int overlap = 16;
  while (join) {
    join = false;
    std::vector<QRect> tmp;
    for (QRect area : areas) {
      // take an area and try to join with something in tmp
      QRect enlArea(area.adjusted(-overlap, -overlap, overlap, overlap));
      bool intersected = false;
      std::vector<QRect> tmp2;
      for (QRect ta : tmp) {
        QRect enlTA(ta.adjusted(-overlap, -overlap, overlap, overlap));
        if (enlArea.intersects(enlTA)) {
          intersected = true;
          join = true;
          tmp2.push_back(area.united(ta));
        } else {
          tmp2.push_back(ta);
        }
      }
      if (!intersected) {
        tmp2.push_back(area);
      }
      tmp = tmp2;
    }
    areas = tmp;
  }

  const auto percent = (float) (sensitivity / 100.);
  if (percent < 1.) {
    for (QRect& area : areas) {
      int word_width = area.width() >> 5;

      int left = area.left();
      int left_word = left >> 5;
      int right = area.x() + area.width();
      int right_word = right >> 5;
      int top = area.top();
      int bottom = area.bottom();

      const uint32_t* pdata = mask.data();

      const auto criterium = (int) (area.width() * percent);
      const auto criterium_word = (int) (word_width * percent);

      // cut the dirty upper lines
      for (int y = top; y < bottom; y++) {
        line = pdata + wpl * y;

        int mword = 0;

        for (int k = left_word; k < right_word; k++) {
          if (!line[k]) {
            mword++;  // count the totally white words
          }
        }

        if (mword > criterium_word) {
          area.setTop(y);
          break;
        }
      }

      // cut the dirty bottom lines
      for (int y = bottom; y > top; y--) {
        line = pdata + wpl * y;

        int mword = 0;

        for (int k = left_word; k < right_word; k++) {
          if (!line[k]) {
            mword++;
          }
        }

        if (mword > criterium_word) {
          area.setBottom(y);
          break;
        }
      }

      for (int x = left; x < right; x++) {
        int mword = 0;

        for (int y = top; y < bottom; y++) {
          if (WHITE == mask.getPixel(x, y)) {
            mword++;
          }
        }

        if (mword > criterium) {
          area.setLeft(x);
          break;
        }
      }

      for (int x = right; x > left; x--) {
        int mword = 0;

        for (int y = top; y < bottom; y++) {
          if (WHITE == mask.getPixel(x, y)) {
            mword++;
          }
        }

        if (mword > criterium) {
          area.setRight(x);
          break;
        }
      }

      area = area.intersected(mask.rect());
    }
  }

  return areas;
}

void applyAffineTransform(QImage& image, const QTransform& xform, const QColor& outside_color) {
  if (xform.isIdentity()) {
    return;
  }
  image = transform(image, xform, image.rect(), OutsidePixels::assumeWeakColor(outside_color));
}

void applyAffineTransform(BinaryImage& image, const QTransform& xform, const BWColor outside_color) {
  if (xform.isIdentity()) {
    return;
  }
  const QColor color = (outside_color == BLACK) ? Qt::black : Qt::white;
  QImage converted = image.toQImage();
  applyAffineTransform(converted, xform, color);
  image = BinaryImage(converted);
}

void hitMissReplaceAllDirections(BinaryImage& img,
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
}

QSize calcLocalWindowSize(const Dpi& dpi) {
  const QSizeF size_mm(3, 30);
  const QSizeF size_inch(size_mm * constants::MM2INCH);
  const QSizeF size_pixels_f(dpi.horizontal() * size_inch.width(), dpi.vertical() * size_inch.height());
  QSize size_pixels(size_pixels_f.toSize());

  if (size_pixels.width() < 3) {
    size_pixels.setWidth(3);
  }
  if (size_pixels.height() < 3) {
    size_pixels.setHeight(3);
  }

  return size_pixels;
}

void movePointToTopMargin(BinaryImage& bw_image, XSpline& spline, int idx) {
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

void movePointToBottomMargin(BinaryImage& bw_image, XSpline& spline, int idx) {
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

void drawPoint(QImage& image, const QPointF& pt) {
  QPoint pts = pt.toPoint();

  for (int i = pts.x() - 10; i < pts.x() + 10; i++) {
    for (int j = pts.y() - 10; j < pts.y() + 10; j++) {
      QPoint p1(i, j);

      image.setPixel(p1, qRgb(255, 0, 0));
    }
  }
}

void movePointToTopMargin(BinaryImage& bw_image, std::vector<QPointF>& polyline, int idx) {
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

void movePointToBottomMargin(BinaryImage& bw_image, std::vector<QPointF>& polyline, int idx) {
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

inline float vertBorderSkewAngle(const QPointF& top, const QPointF& bottom) {
  return static_cast<float>(std::abs(std::atan((bottom.x() - top.x()) / (bottom.y() - top.y())) * 180.0 / M_PI));
}

QImage smoothToGrayscale(const QImage& src, const Dpi& dpi) {
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

QSize from300dpi(const QSize& size, const Dpi& target_dpi) {
  const double hscale = target_dpi.horizontal() / 300.0;
  const double vscale = target_dpi.vertical() / 300.0;
  const int width = qRound(size.width() * hscale);
  const int height = qRound(size.height() * vscale);

  return QSize(std::max(1, width), std::max(1, height));
}

QSize to300dpi(const QSize& size, const Dpi& source_dpi) {
  const double hscale = 300.0 / source_dpi.horizontal();
  const double vscale = 300.0 / source_dpi.vertical();
  const int width = qRound(size.width() * hscale);
  const int height = qRound(size.height() * vscale);

  return QSize(std::max(1, width), std::max(1, height));
}
}  // namespace

std::unique_ptr<OutputImage> OutputGenerator::Processor::buildEmptyImage() const {
  OutputImageBuilder imageBuilder;

  BinaryImage emptyImage(m_targetSize, WHITE);
  imageBuilder.setImage(emptyImage.toQImage());
  if (m_renderParams.splitOutput()) {
    imageBuilder.setForegroundMask(emptyImage);
    if (m_renderParams.originalBackground()) {
      imageBuilder.setBackgroundMask(emptyImage);
    }
  }
  return imageBuilder.build();
}

void OutputGenerator::Processor::updateBlackOnWhite(const FilterData& input) {
  QSettings appSettings;
  Params params = m_settings->getParams(m_pageId);
  if ((appSettings.value("settings/blackOnWhiteDetection", true).toBool()
       && appSettings.value("settings/blackOnWhiteDetectionAtOutput", true).toBool())
      && !m_settings->getOutputProcessingParams(m_pageId).isBlackOnWhiteSetManually()) {
    if (params.isBlackOnWhite() != input.isBlackOnWhite()) {
      params.setBlackOnWhite(input.isBlackOnWhite());
      m_settings->setParams(m_pageId, params);
    }
  }
  m_blackOnWhite = params.isBlackOnWhite();
}

void OutputGenerator::Processor::initFilterData(const FilterData& input) {
  updateBlackOnWhite(input);

  m_inputGrayImage = m_blackOnWhite ? input.grayImage() : input.grayImage().inverted();
  m_inputOrigImage = input.origImage();
  if (!m_blackOnWhite) {
    m_inputOrigImage.invertPixels();
  }
  m_colorOriginal = !m_inputOrigImage.allGray();
}

std::unique_ptr<OutputImage> OutputGenerator::Processor::processImpl(ZoneSet& picture_zones,
                                                                     const ZoneSet& fill_zones,
                                                                     DistortionModel& distortion_model,
                                                                     const DepthPerception& depth_perception,
                                                                     BinaryImage* auto_picture_mask,
                                                                     BinaryImage* speckles_image) {
  if (m_blank) {
    return buildEmptyImage();
  }

  m_outsideBackgroundColor = BackgroundColorCalculator::calcDominantBackgroundColor(
      m_colorOriginal ? m_inputOrigImage : m_inputGrayImage, m_outCropAreaInOriginalCs);

  if ((m_dewarpingOptions.dewarpingMode() == AUTO) || (m_dewarpingOptions.dewarpingMode() == MARGINAL)
      || ((m_dewarpingOptions.dewarpingMode() == MANUAL) && distortion_model.isValid())) {
    return processWithDewarping(picture_zones, fill_zones, distortion_model, depth_perception, auto_picture_mask,
                                speckles_image);
  } else {
    return processWithoutDewarping(picture_zones, fill_zones, auto_picture_mask, speckles_image);
  }
}

std::unique_ptr<OutputImage> OutputGenerator::Processor::processWithoutDewarping(ZoneSet& picture_zones,
                                                                                 const ZoneSet& fill_zones,
                                                                                 BinaryImage* auto_picture_mask,
                                                                                 BinaryImage* speckles_image) {
  OutputImageBuilder imageBuilder;

  QImage maybe_normalized = transformToWorkingCs(m_renderParams.normalizeIllumination());
  if (m_dbg) {
    m_dbg->add(maybe_normalized, "maybe_normalized");
  }

  if (m_renderParams.normalizeIllumination()) {
    m_outsideBackgroundColor
        = BackgroundColorCalculator::calcDominantBackgroundColor(maybe_normalized, m_outCropAreaInWorkingCs);
  }

  m_status.throwIfCancelled();

  if (m_renderParams.binaryOutput()) {
    QImage maybe_smoothed;
    // We only do smoothing if we are going to do binarization later.
    if (!m_renderParams.needSavitzkyGolaySmoothing()) {
      maybe_smoothed = maybe_normalized;
    } else {
      maybe_smoothed = smoothToGrayscale(maybe_normalized, m_dpi);
      if (m_dbg) {
        m_dbg->add(maybe_smoothed, "smoothed");
      }
      m_status.throwIfCancelled();
    }

    // don't destroy as it's needed for color segmentation
    if (!m_renderParams.needColorSegmentation()) {
      maybe_normalized = QImage();
    }

    BinaryImage bw_content = binarize(maybe_smoothed, m_contentAreaInWorkingCs);
    maybe_smoothed = QImage();  // Save memory.
    if (m_dbg) {
      m_dbg->add(bw_content, "binarized_and_cropped");
    }
    m_status.throwIfCancelled();

    if (m_renderParams.needMorphologicalSmoothing()) {
      morphologicalSmoothInPlace(bw_content);
      m_status.throwIfCancelled();
    }

    BinaryImage dst(m_targetSize, WHITE);
    rasterOp<RopSrc>(dst, m_croppedContentRect, bw_content, m_contentRectInWorkingCs.topLeft());
    bw_content.release();  // Save memory.

    // It's important to keep despeckling the very last operation
    // affecting the binary part of the output. That's because
    // we will be reconstructing the input to this despeckling
    // operation from the final output file.
    maybeDespeckleInPlace(dst, m_outRect, m_outRect, m_despeckleLevel, speckles_image, m_dpi);

    if (!m_renderParams.needColorSegmentation()) {
      if (!m_blackOnWhite) {
        dst.invert();
      }

      applyFillZonesInPlace(dst, fill_zones, m_xform.transform());

      imageBuilder.setImage(dst.toQImage());
    } else {
      QImage segmented_image;
      {
        QImage color_image(m_targetSize, maybe_normalized.format());
        if (maybe_normalized.format() == QImage::Format_Indexed8) {
          color_image.setColorTable(maybe_normalized.colorTable());
        }
        color_image.fill(Qt::white);
        drawOver(color_image, m_croppedContentRect, maybe_normalized, m_contentRectInWorkingCs);
        maybe_normalized = QImage();

        segmented_image = segmentImage(dst, color_image);
        dst.release();
      }

      if (m_renderParams.posterize()) {
        segmented_image = posterizeImage(segmented_image, m_outsideBackgroundColor);
      }

      if (!m_blackOnWhite) {
        segmented_image.invertPixels();
      }

      applyFillZonesInPlace(segmented_image, fill_zones, m_xform.transform(), false);
      if (m_dbg) {
        m_dbg->add(segmented_image, "segmented_with_fill_zones");
      }
      m_status.throwIfCancelled();

      imageBuilder.setImage(segmented_image);
    }

    return imageBuilder.build();
  }

  BinaryImage bw_content_mask_output;
  BinaryImage bw_content_output;
  if (m_renderParams.mixedOutput()) {
    BinaryImage bw_mask(m_workingBoundingRect.size(), BLACK);
    processPictureZones(bw_mask, picture_zones, GrayImage(maybe_normalized));
    if (m_dbg) {
      m_dbg->add(bw_mask, "bw_mask");
    }
    m_status.throwIfCancelled();

    if (auto_picture_mask) {
      if (auto_picture_mask->size() != m_targetSize) {
        BinaryImage(m_targetSize).swap(*auto_picture_mask);
      }
      auto_picture_mask->fill(BLACK);
      rasterOp<RopSrc>(*auto_picture_mask, m_croppedContentRect, bw_mask, m_contentRectInWorkingCs.topLeft());
    }
    m_status.throwIfCancelled();

    modifyBinarizationMask(bw_mask, m_workingBoundingRect, picture_zones);
    fillMarginsInPlace(bw_mask, m_contentAreaInWorkingCs, BLACK);
    if (m_dbg) {
      m_dbg->add(bw_mask, "bw_mask with zones");
    }
    m_status.throwIfCancelled();

    if (m_renderParams.needBinarization()) {
      QImage maybe_smoothed;
      if (!m_renderParams.needSavitzkyGolaySmoothing()) {
        maybe_smoothed = maybe_normalized;
      } else {
        maybe_smoothed = smoothToGrayscale(maybe_normalized, m_dpi);
        if (m_dbg) {
          m_dbg->add(maybe_smoothed, "smoothed");
        }
        m_status.throwIfCancelled();
      }

      BinaryImage bw_mask_filled(bw_mask);
      fillMarginsInPlace(bw_mask_filled, m_contentAreaInWorkingCs, WHITE);
      BinaryImage bw_content = binarize(maybe_smoothed, bw_mask_filled);
      bw_mask_filled.release();
      maybe_smoothed = QImage();  // Save memory.
      if (m_dbg) {
        m_dbg->add(bw_content, "binarized_and_cropped");
      }
      m_status.throwIfCancelled();

      if (m_renderParams.needMorphologicalSmoothing()) {
        morphologicalSmoothInPlace(bw_content);
      }

      // It's important to keep despeckling the very last operation
      // affecting the binary part of the output. That's because
      // we will be reconstructing the input to this despeckling
      // operation from the final output file.
      maybeDespeckleInPlace(bw_content, m_workingBoundingRect, m_croppedContentRect, m_despeckleLevel, speckles_image,
                            m_dpi);

      if (!m_renderParams.normalizeIlluminationColor()) {
        m_outsideBackgroundColor = BackgroundColorCalculator::calcDominantBackgroundColor(
            m_colorOriginal ? m_inputOrigImage : m_inputGrayImage, m_outCropAreaInOriginalCs);
        maybe_normalized = transformToWorkingCs(false);
      }

      if (!m_renderParams.needColorSegmentation()) {
        if (m_renderParams.originalBackground()) {
          combineImages(maybe_normalized, bw_content);
        } else {
          combineImages(maybe_normalized, bw_content, bw_mask);
        }
      } else {
        QImage segmented_image = segmentImage(bw_content, maybe_normalized, &bw_mask);
        if (m_renderParams.posterize()) {
          segmented_image = posterizeImage(segmented_image, m_outsideBackgroundColor);
        }

        if (m_renderParams.originalBackground()) {
          combineImages(maybe_normalized, segmented_image);
        } else {
          combineImages(maybe_normalized, segmented_image, bw_mask);
        }
      }

      if (m_dbg) {
        m_dbg->add(maybe_normalized, "combined");
      }

      if (m_renderParams.originalBackground()) {
        reserveBlackAndWhite(maybe_normalized, bw_content.inverted());
      } else {
        reserveBlackAndWhite(maybe_normalized, bw_mask.inverted());
      }
      m_status.throwIfCancelled();

      if (m_renderParams.originalBackground()) {
        bw_content_output = BinaryImage(m_targetSize, WHITE);
        rasterOp<RopSrc>(bw_content_output, m_croppedContentRect, bw_content, m_contentRectInWorkingCs.topLeft());
      }
    }

    bw_content_mask_output = BinaryImage(m_targetSize, BLACK);
    rasterOp<RopSrc>(bw_content_mask_output, m_croppedContentRect, bw_mask, m_contentRectInWorkingCs.topLeft());
  }

  assert(!m_targetSize.isEmpty());
  QImage dst(m_targetSize, maybe_normalized.format());
  if (maybe_normalized.format() == QImage::Format_Indexed8) {
    dst.setColorTable(maybe_normalized.colorTable());
  }

  if (dst.isNull()) {
    // Both the constructor and setColorTable() above can leave the image null.
    throw std::bad_alloc();
  }

  if (m_renderParams.needBinarization() && !m_renderParams.originalBackground()) {
    m_outsideBackgroundColor = Qt::white;
  } else if (m_colorParams.colorCommonOptions().getFillingColor() == FILL_WHITE) {
    m_outsideBackgroundColor = m_blackOnWhite ? Qt::white : Qt::black;
  }
  fillMarginsInPlace(maybe_normalized, m_contentAreaInWorkingCs, m_outsideBackgroundColor);
  dst.fill(m_outsideBackgroundColor);

  drawOver(dst, m_croppedContentRect, maybe_normalized, m_contentRectInWorkingCs);
  maybe_normalized = QImage();

  if (!m_blackOnWhite) {
    dst.invertPixels();
  }

  if (m_renderParams.mixedOutput() && m_renderParams.needBinarization()) {
    applyFillZonesToMixedInPlace(dst, fill_zones, m_xform.transform(), bw_content_mask_output,
                                 !m_renderParams.needColorSegmentation());
  } else {
    applyFillZonesInPlace(dst, fill_zones, m_xform.transform());
  }
  if (m_renderParams.originalBackground()) {
    applyFillZonesToMask(bw_content_output, fill_zones, m_xform.transform());
    rasterOp<RopAnd<RopSrc, RopDst>>(bw_content_output, bw_content_mask_output);
  }
  if (m_dbg) {
    m_dbg->add(dst, "fill_zones");
  }
  m_status.throwIfCancelled();

  if (m_renderParams.posterize() && !m_renderParams.needBinarization()) {
    dst = posterizeImage(dst);
  }

  if (m_renderParams.splitOutput()) {
    imageBuilder.setForegroundType(getForegroundType());
    if (!m_renderParams.originalBackground()) {
      imageBuilder.setForegroundMask(bw_content_mask_output);
    } else {
      imageBuilder.setForegroundMask(bw_content_output);
      imageBuilder.setBackgroundMask(bw_content_mask_output.inverted());
    }
  }

  return imageBuilder.setImage(dst).build();
}

std::unique_ptr<OutputImage> OutputGenerator::Processor::processWithDewarping(ZoneSet& picture_zones,
                                                                              const ZoneSet& fill_zones,
                                                                              DistortionModel& distortion_model,
                                                                              const DepthPerception& depth_perception,
                                                                              BinaryImage* auto_picture_mask,
                                                                              BinaryImage* speckles_image) {
  OutputImageBuilder imageBuilder;

  // The output we would get if dewarping was turned off, except always grayscale.
  // Used for automatic picture detection and binarization threshold calculation.
  // This image corresponds to the area of workingBoundingRect.
  GrayImage warped_gray_output;
  if (!m_renderParams.normalizeIllumination()) {
    warped_gray_output = transformToGray(m_inputGrayImage, m_xform.transform(), m_workingBoundingRect,
                                         OutsidePixels::assumeWeakColor(m_outsideBackgroundColor));
  } else {
    warped_gray_output = normalizeIlluminationGray(m_inputGrayImage, m_preCropAreaInOriginalCs, m_xform.transform(),
                                                   m_workingBoundingRect);
  }

  // Original image, but:
  // 1. In a format we can handle, that is grayscale, RGB32, ARGB32
  // 2. With illumination normalized over the content area, if required.
  // 3. With margins filled with white, if required.
  QImage normalized_original;
  const QTransform working_to_orig
      = QTransform().translate(m_workingBoundingRect.left(), m_workingBoundingRect.top()) * m_xform.transformBack();
  if (!m_renderParams.normalizeIllumination()) {
    normalized_original = (m_colorOriginal) ? m_inputOrigImage : m_inputGrayImage;
  } else {
    GrayImage normalized_gray = transformToGray(warped_gray_output, working_to_orig, m_inputOrigImage.rect(),
                                                OutsidePixels::assumeWeakColor(m_outsideBackgroundColor));
    if (!m_colorOriginal) {
      normalized_original = normalized_gray;
    } else {
      normalized_original = m_inputOrigImage;
      adjustBrightnessGrayscale(normalized_original, normalized_gray);
      if (m_dbg) {
        m_dbg->add(normalized_original, "norm_illum_color");
      }
    }

    m_outsideBackgroundColor
        = BackgroundColorCalculator::calcDominantBackgroundColor(normalized_original, m_outCropAreaInOriginalCs);
  }

  m_status.throwIfCancelled();

  // Picture mask (white indicate a picture) in the same coordinates as
  // warped_gray_output.  Only built for Mixed mode.
  BinaryImage warped_bw_mask;
  if (m_renderParams.mixedOutput()) {
    warped_bw_mask = BinaryImage(m_workingBoundingRect.size(), BLACK);
    processPictureZones(warped_bw_mask, picture_zones, warped_gray_output);
    if (m_dbg) {
      m_dbg->add(warped_bw_mask, "warped_bw_mask");
    }
    m_status.throwIfCancelled();

    if (auto_picture_mask) {
      if (auto_picture_mask->size() != m_targetSize) {
        BinaryImage(m_targetSize).swap(*auto_picture_mask);
      }
      auto_picture_mask->fill(BLACK);
      rasterOp<RopSrc>(*auto_picture_mask, m_croppedContentRect, warped_bw_mask, m_contentRectInWorkingCs.topLeft());
    }
    m_status.throwIfCancelled();

    modifyBinarizationMask(warped_bw_mask, m_workingBoundingRect, picture_zones);
    if (m_dbg) {
      m_dbg->add(warped_bw_mask, "warped_bw_mask with zones");
    }
    m_status.throwIfCancelled();
  }

  if (m_dewarpingOptions.dewarpingMode() == AUTO) {
    distortion_model = buildAutoDistortionModel(warped_gray_output, working_to_orig);
  } else if (m_dewarpingOptions.dewarpingMode() == MARGINAL) {
    distortion_model = buildMarginalDistortionModel();
  }
  warped_gray_output = GrayImage();  // Save memory.

  QTransform rotate_xform;
  QImage dewarped;
  try {
    dewarped = dewarp(QTransform(), normalized_original, m_xform.transform(), distortion_model, depth_perception,
                      m_outsideBackgroundColor);

    const double deskew_angle = -findSkew(dewarped);
    rotate_xform = Utils::rotate(deskew_angle, m_outRect);
    m_dewarpingOptions.setPostDeskewAngle(deskew_angle);
  } catch (const std::runtime_error&) {
    // Probably an impossible distortion model.  Let's fall back to a trivial one.
    setupTrivialDistortionModel(distortion_model);
    m_dewarpingOptions.setPostDeskewAngle(.0);

    dewarped = dewarp(QTransform(), normalized_original, m_xform.transform(), distortion_model, depth_perception,
                      m_outsideBackgroundColor);
  }
  normalized_original = QImage();  // Save memory.
  m_settings->setDewarpingOptions(m_pageId, m_dewarpingOptions);
  applyAffineTransform(dewarped, rotate_xform, m_outsideBackgroundColor);
  normalized_original = QImage();  // Save memory.
  if (m_dbg) {
    m_dbg->add(dewarped, "dewarped");
  }
  m_status.throwIfCancelled();

  std::shared_ptr<DewarpingPointMapper> mapper(new DewarpingPointMapper(
      distortion_model, depth_perception.value(), m_xform.transform(), m_croppedContentRect, rotate_xform));
  const boost::function<QPointF(const QPointF&)> orig_to_output(
      boost::bind(&DewarpingPointMapper::mapToDewarpedSpace, mapper, _1));

  BinaryImage dewarping_content_area_mask(m_inputGrayImage.size(), BLACK);
  {
    fillMarginsInPlace(dewarping_content_area_mask, m_contentAreaInOriginalCs, WHITE);
    dewarping_content_area_mask
        = BinaryImage(dewarp(QTransform(), dewarping_content_area_mask.toQImage(), m_xform.transform(),
                             distortion_model, depth_perception, Qt::white));
  }
  applyAffineTransform(dewarping_content_area_mask, rotate_xform, WHITE);

  if (m_renderParams.binaryOutput()) {
    QImage dewarped_and_maybe_smoothed;
    // We only do smoothing if we are going to do binarization later.
    if (!m_renderParams.needSavitzkyGolaySmoothing()) {
      dewarped_and_maybe_smoothed = dewarped;
    } else {
      dewarped_and_maybe_smoothed = smoothToGrayscale(dewarped, m_dpi);
      if (m_dbg) {
        m_dbg->add(dewarped_and_maybe_smoothed, "smoothed");
      }
      m_status.throwIfCancelled();
    }

    // don't destroy as it's needed for color segmentation
    if (!m_renderParams.needColorSegmentation()) {
      dewarped = QImage();
    }

    BinaryImage dewarped_bw_content = binarize(dewarped_and_maybe_smoothed, dewarping_content_area_mask);
    dewarped_and_maybe_smoothed = QImage();  // Save memory.
    if (m_dbg) {
      m_dbg->add(dewarped_bw_content, "dewarped_bw_content");
    }
    m_status.throwIfCancelled();

    if (m_renderParams.needMorphologicalSmoothing()) {
      morphologicalSmoothInPlace(dewarped_bw_content);
      m_status.throwIfCancelled();
    }

    // It's important to keep despeckling the very last operation
    // affecting the binary part of the output. That's because
    // we will be reconstructing the input to this despeckling
    // operation from the final output file.
    maybeDespeckleInPlace(dewarped_bw_content, m_outRect, m_outRect, m_despeckleLevel, speckles_image, m_dpi);

    if (!m_renderParams.needColorSegmentation()) {
      if (!m_blackOnWhite) {
        dewarped_bw_content.invert();
      }

      applyFillZonesInPlace(dewarped_bw_content, fill_zones, orig_to_output);

      imageBuilder.setImage(dewarped_bw_content.toQImage());
    } else {
      QImage segmented_image = segmentImage(dewarped_bw_content, dewarped);
      dewarped = QImage();
      dewarped_bw_content.release();

      if (m_renderParams.posterize()) {
        segmented_image = posterizeImage(segmented_image, m_outsideBackgroundColor);
      }

      if (!m_blackOnWhite) {
        segmented_image.invertPixels();
      }

      applyFillZonesInPlace(segmented_image, fill_zones, orig_to_output, false);
      if (m_dbg) {
        m_dbg->add(segmented_image, "segmented_with_fill_zones");
      }
      m_status.throwIfCancelled();

      imageBuilder.setImage(segmented_image);
    }

    return imageBuilder.build();
  }

  BinaryImage dewarped_bw_mask;
  BinaryImage dewarped_bw_content;
  if (m_renderParams.mixedOutput()) {
    const QTransform orig_to_working_cs
        = m_xform.transform() * QTransform().translate(-m_workingBoundingRect.left(), -m_workingBoundingRect.top());
    QTransform working_to_output_cs = QTransform().translate(m_workingBoundingRect.left(), m_workingBoundingRect.top());
    dewarped_bw_mask = BinaryImage(dewarp(orig_to_working_cs, warped_bw_mask.toQImage(), working_to_output_cs,
                                          distortion_model, depth_perception, Qt::black));
    warped_bw_mask.release();
    applyAffineTransform(dewarped_bw_mask, rotate_xform, BLACK);
    fillMarginsInPlace(dewarped_bw_mask, dewarping_content_area_mask, BLACK);
    if (m_dbg) {
      m_dbg->add(dewarped_bw_mask, "dewarped_bw_mask");
    }
    m_status.throwIfCancelled();

    if (m_renderParams.needBinarization()) {
      QImage dewarped_and_maybe_smoothed;
      if (!m_renderParams.needSavitzkyGolaySmoothing()) {
        dewarped_and_maybe_smoothed = dewarped;
      } else {
        dewarped_and_maybe_smoothed = smoothToGrayscale(dewarped, m_dpi);
        if (m_dbg) {
          m_dbg->add(dewarped_and_maybe_smoothed, "smoothed");
        }
        m_status.throwIfCancelled();
      }

      BinaryImage dewarped_bw_mask_filled(dewarped_bw_mask);
      fillMarginsInPlace(dewarped_bw_mask_filled, dewarping_content_area_mask, WHITE);
      dewarped_bw_content = binarize(dewarped_and_maybe_smoothed, dewarped_bw_mask_filled);
      dewarped_bw_mask_filled.release();
      dewarped_and_maybe_smoothed = QImage();  // Save memory.
      if (m_dbg) {
        m_dbg->add(dewarped_bw_content, "dewarped_bw_content");
      }
      m_status.throwIfCancelled();

      if (m_renderParams.needMorphologicalSmoothing()) {
        morphologicalSmoothInPlace(dewarped_bw_content);
      }

      // It's important to keep despeckling the very last operation
      // affecting the binary part of the output. That's because
      // we will be reconstructing the input to this despeckling
      // operation from the final output file.
      maybeDespeckleInPlace(dewarped_bw_content, m_outRect, m_croppedContentRect, m_despeckleLevel, speckles_image,
                            m_dpi);

      if (!m_renderParams.normalizeIlluminationColor()) {
        m_outsideBackgroundColor = BackgroundColorCalculator::calcDominantBackgroundColor(
            m_colorOriginal ? m_inputOrigImage : m_inputGrayImage, m_outCropAreaInOriginalCs);

        {
          QImage orig_without_illumination;
          if (m_colorOriginal) {
            orig_without_illumination = m_inputOrigImage;
          } else {
            orig_without_illumination = m_inputGrayImage;
          }
          dewarped = dewarp(QTransform(), orig_without_illumination, m_xform.transform(), distortion_model,
                            depth_perception, m_outsideBackgroundColor);
        }
        applyAffineTransform(dewarped, rotate_xform, m_outsideBackgroundColor);
        m_status.throwIfCancelled();
      }

      if (!m_renderParams.needColorSegmentation()) {
        if (m_renderParams.originalBackground()) {
          combineImages(dewarped, dewarped_bw_content);
        } else {
          combineImages(dewarped, dewarped_bw_content, dewarped_bw_mask);
        }
      } else {
        QImage segmented_image = segmentImage(dewarped_bw_content, dewarped, &dewarped_bw_mask);
        if (m_renderParams.posterize()) {
          segmented_image = posterizeImage(segmented_image, m_outsideBackgroundColor);
        }

        if (m_renderParams.originalBackground()) {
          combineImages(dewarped, segmented_image);
        } else {
          combineImages(dewarped, segmented_image, dewarped_bw_mask);
        }
      }

      if (m_dbg) {
        m_dbg->add(dewarped, "combined");
      }

      if (m_renderParams.originalBackground()) {
        reserveBlackAndWhite(dewarped, dewarped_bw_content.inverted());
      } else {
        reserveBlackAndWhite(dewarped, dewarped_bw_mask.inverted());
      }
      m_status.throwIfCancelled();
    }
  }

  if (m_renderParams.needBinarization() && !m_renderParams.originalBackground()) {
    m_outsideBackgroundColor = Qt::white;
  } else if (m_colorParams.colorCommonOptions().getFillingColor() == FILL_WHITE) {
    m_outsideBackgroundColor = m_blackOnWhite ? Qt::white : Qt::black;
  }
  fillMarginsInPlace(dewarped, dewarping_content_area_mask, m_outsideBackgroundColor);

  if (!m_blackOnWhite) {
    dewarped.invertPixels();
  }

  if (m_renderParams.mixedOutput() && m_renderParams.needBinarization()) {
    applyFillZonesToMixedInPlace(dewarped, fill_zones, orig_to_output, dewarped_bw_mask,
                                 !m_renderParams.needColorSegmentation());
  } else {
    applyFillZonesInPlace(dewarped, fill_zones, orig_to_output);
  }
  if (m_renderParams.originalBackground()) {
    applyFillZonesToMask(dewarped_bw_content, fill_zones, orig_to_output);
    rasterOp<RopAnd<RopSrc, RopDst>>(dewarped_bw_content, dewarped_bw_mask);
  }
  if (m_dbg) {
    m_dbg->add(dewarped, "fill_zones");
  }
  m_status.throwIfCancelled();

  if (m_renderParams.posterize() && !m_renderParams.needBinarization()) {
    dewarped = posterizeImage(dewarped);
  }

  if (m_renderParams.splitOutput()) {
    imageBuilder.setForegroundType(getForegroundType());
    if (!m_renderParams.originalBackground()) {
      imageBuilder.setForegroundMask(dewarped_bw_mask);
    } else {
      imageBuilder.setForegroundMask(dewarped_bw_content);
      imageBuilder.setBackgroundMask(dewarped_bw_mask.inverted());
    }
  }

  return imageBuilder.setImage(dewarped).build();
}

GrayImage OutputGenerator::Processor::normalizeIlluminationGray(const QImage& input,
                                                                const QPolygonF& area_to_consider,
                                                                const QTransform& xform,
                                                                const QRect& target_rect,
                                                                GrayImage* background) const {
  GrayImage to_be_normalized = transformToGray(input, xform, target_rect, OutsidePixels::assumeWeakNearest());
  if (m_dbg) {
    m_dbg->add(to_be_normalized, "to_be_normalized");
  }

  m_status.throwIfCancelled();

  QPolygonF transformed_consideration_area = xform.map(area_to_consider);
  transformed_consideration_area.translate(-target_rect.topLeft());

  const PolynomialSurface bg_ps = estimateBackground(to_be_normalized, transformed_consideration_area, m_status, m_dbg);
  m_status.throwIfCancelled();

  GrayImage bg_img(bg_ps.render(to_be_normalized.size()));
  if (m_dbg) {
    m_dbg->add(bg_img, "background");
  }
  if (background) {
    *background = bg_img;
  }
  m_status.throwIfCancelled();

  grayRasterOp<RaiseAboveBackground>(bg_img, to_be_normalized);
  if (m_dbg) {
    m_dbg->add(bg_img, "normalized_illumination");
  }
  m_status.throwIfCancelled();

  return bg_img;
}

BinaryImage OutputGenerator::Processor::estimateBinarizationMask(const GrayImage& gray_source,
                                                                 const QRect& source_rect,
                                                                 const QRect& source_sub_rect) const {
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
    relative_subrect.moveTopLeft(source_sub_rect.topLeft() - source_rect.topLeft());

    const int stride = gray_source.stride();
    const int offset = relative_subrect.top() * stride + relative_subrect.left();

    trimmed_image = GrayImage(QImage(gray_source.data() + offset, relative_subrect.width(), relative_subrect.height(),
                                     stride, QImage::Format_Indexed8));
  }

  m_status.throwIfCancelled();

  const QSize downscaled_size(to300dpi(trimmed_image.size(), m_dpi));

  // A 300dpi version of trimmed_image.
  GrayImage downscaled_input(scaleToGray(trimmed_image, downscaled_size));
  trimmed_image = GrayImage();  // Save memory.
  m_status.throwIfCancelled();

  // Light areas indicate pictures.
  GrayImage picture_areas(detectPictures(downscaled_input));
  downscaled_input = GrayImage();  // Save memory.
  m_status.throwIfCancelled();

  const BinaryThreshold threshold(48);
  // Scale back to original size.
  picture_areas = scaleToGray(picture_areas, source_sub_rect.size());

  return BinaryImage(picture_areas, threshold);
}

void OutputGenerator::Processor::modifyBinarizationMask(BinaryImage& bw_mask,
                                                        const QRect& mask_rect,
                                                        const ZoneSet& zones) const {
  QTransform xform = m_xform.transform();
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


/**
 * Set up a distortion model corresponding to the content rect,
 * which will result in no distortion correction.
 */
void OutputGenerator::Processor::setupTrivialDistortionModel(DistortionModel& distortion_model) const {
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
  top_polyline.push_back(poly[0]);     // top-left
  top_polyline.push_back(poly[1]);     // top-right
  bottom_polyline.push_back(poly[3]);  // bottom-left
  bottom_polyline.push_back(poly[2]);  // bottom-right
  distortion_model.setTopCurve(Curve(top_polyline));
  distortion_model.setBottomCurve(Curve(bottom_polyline));
}

CylindricalSurfaceDewarper OutputGenerator::Processor::createDewarper(const DistortionModel& distortion_model,
                                                                      const QTransform& distortion_model_to_target,
                                                                      double depth_perception) {
  if (distortion_model_to_target.isIdentity()) {
    return CylindricalSurfaceDewarper(distortion_model.topCurve().polyline(), distortion_model.bottomCurve().polyline(),
                                      depth_perception);
  }

  std::vector<QPointF> top_polyline(distortion_model.topCurve().polyline());
  std::vector<QPointF> bottom_polyline(distortion_model.bottomCurve().polyline());
  for (QPointF& pt : top_polyline) {
    pt = distortion_model_to_target.map(pt);
  }
  for (QPointF& pt : bottom_polyline) {
    pt = distortion_model_to_target.map(pt);
  }

  return CylindricalSurfaceDewarper(top_polyline, bottom_polyline, depth_perception);
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
QImage OutputGenerator::Processor::dewarp(const QTransform& orig_to_src,
                                          const QImage& src,
                                          const QTransform& src_to_output,
                                          const DistortionModel& distortion_model,
                                          const DepthPerception& depth_perception,
                                          const QColor& bg_color) const {
  const CylindricalSurfaceDewarper dewarper(createDewarper(distortion_model, orig_to_src, depth_perception.value()));

  // Model domain is a rectangle in output image coordinates that
  // will be mapped to our curved quadrilateral.
  const QRect model_domain(distortion_model.modelDomain(dewarper, orig_to_src * src_to_output, m_outRect).toRect());
  if (model_domain.isEmpty()) {
    GrayImage out(src.size());
    out.fill(0xff);  // white

    return out;
  }

  return RasterDewarper::dewarp(src, m_outRect.size(), dewarper, model_domain, bg_color);
}

GrayImage OutputGenerator::Processor::detectPictures(const GrayImage& input_300dpi) const {
  // We stretch the range of gray levels to cover the whole
  // range of [0, 255].  We do it because we want text
  // and background to be equally far from the center
  // of the whole range.  Otherwise text printed with a big
  // font will be considered a picture.
  GrayImage stretched(stretchGrayRange(input_300dpi, 0.01, 0.01));
  if (m_dbg) {
    m_dbg->add(stretched, "stretched");
  }

  m_status.throwIfCancelled();

  GrayImage eroded(erodeGray(stretched, QSize(3, 3), 0x00));
  if (m_dbg) {
    m_dbg->add(eroded, "eroded");
  }

  m_status.throwIfCancelled();

  GrayImage dilated(dilateGray(stretched, QSize(3, 3), 0xff));
  if (m_dbg) {
    m_dbg->add(dilated, "dilated");
  }

  stretched = GrayImage();  // Save memory.
  m_status.throwIfCancelled();

  grayRasterOp<CombineInverted>(dilated, eroded);
  GrayImage gray_gradient(dilated);
  dilated = GrayImage();
  eroded = GrayImage();
  if (m_dbg) {
    m_dbg->add(gray_gradient, "gray_gradient");
  }

  m_status.throwIfCancelled();

  GrayImage marker(erodeGray(gray_gradient, QSize(35, 35), 0x00));
  if (m_dbg) {
    m_dbg->add(marker, "marker");
  }

  m_status.throwIfCancelled();

  seedFillGrayInPlace(marker, gray_gradient, CONN8);
  GrayImage reconstructed(marker);
  marker = GrayImage();
  if (m_dbg) {
    m_dbg->add(reconstructed, "reconstructed");
  }

  m_status.throwIfCancelled();

  grayRasterOp<GRopInvert<GRopSrc>>(reconstructed, reconstructed);
  if (m_dbg) {
    m_dbg->add(reconstructed, "reconstructed_inverted");
  }

  m_status.throwIfCancelled();

  GrayImage holes_filled(createFramedImage(reconstructed.size()));
  seedFillGrayInPlace(holes_filled, reconstructed, CONN8);
  reconstructed = GrayImage();
  if (m_dbg) {
    m_dbg->add(holes_filled, "holes_filled");
  }

  if (m_pictureShapeOptions.isHigherSearchSensitivity()) {
    GrayImage stretched2(stretchGrayRange(holes_filled, 5.0, 0.01));
    if (m_dbg) {
      m_dbg->add(stretched2, "stretched2");
    }

    return stretched2;
  }

  return holes_filled;
}

BinaryThreshold OutputGenerator::Processor::adjustThreshold(BinaryThreshold threshold) const {
  const int adjusted = threshold + m_colorParams.blackWhiteOptions().thresholdAdjustment();

  // Hard-bounding threshold values is necessary for example
  // if all the content went into the picture mask.
  return BinaryThreshold(qBound(30, adjusted, 225));
}

BinaryThreshold OutputGenerator::Processor::calcBinarizationThreshold(const QImage& image,
                                                                      const BinaryImage& mask) const {
  GrayscaleHistogram hist(image, mask);

  return adjustThreshold(BinaryThreshold::otsuThreshold(hist));
}

BinaryThreshold OutputGenerator::Processor::calcBinarizationThreshold(const QImage& image,
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

void OutputGenerator::Processor::morphologicalSmoothInPlace(BinaryImage& bin_img) const {
  // When removing black noise, remove small ones first.

  {
    const char pattern[]
        = "XXX"
          " - "
          "   ";
    hitMissReplaceAllDirections(bin_img, pattern, 3, 3);
  }

  m_status.throwIfCancelled();

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

  m_status.throwIfCancelled();

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

  m_status.throwIfCancelled();

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

  m_status.throwIfCancelled();

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

  m_status.throwIfCancelled();

  {
    const char pattern[]
        = "   "
          "X+X"
          "XXX";
    hitMissReplaceAllDirections(bin_img, pattern, 3, 3);
  }

  if (m_dbg) {
    m_dbg->add(bin_img, "edges_smoothed");
  }
}

BinaryImage OutputGenerator::Processor::binarize(const QImage& image) const {
  if ((image.format() == QImage::Format_Mono) || (image.format() == QImage::Format_MonoLSB)) {
    return BinaryImage(image);
  }

  const BlackWhiteOptions& blackWhiteOptions = m_colorParams.blackWhiteOptions();
  const BinarizationMethod binarizationMethod = blackWhiteOptions.getBinarizationMethod();

  BinaryImage binarized;
  switch (binarizationMethod) {
    case OTSU: {
      GrayscaleHistogram hist(image);
      const BinaryThreshold bw_thresh(BinaryThreshold::otsuThreshold(hist));

      binarized = BinaryImage(image, adjustThreshold(bw_thresh));
      break;
    }
    case SAUVOLA: {
      QSize windowsSize = QSize(blackWhiteOptions.getWindowSize(), blackWhiteOptions.getWindowSize());
      double sauvolaCoef = blackWhiteOptions.getSauvolaCoef();

      binarized = binarizeSauvola(image, windowsSize, sauvolaCoef);
      break;
    }
    case WOLF: {
      QSize windowsSize = QSize(blackWhiteOptions.getWindowSize(), blackWhiteOptions.getWindowSize());
      auto lowerBound = (unsigned char) blackWhiteOptions.getWolfLowerBound();
      auto upperBound = (unsigned char) blackWhiteOptions.getWolfUpperBound();
      double wolfCoef = blackWhiteOptions.getWolfCoef();

      binarized = binarizeWolf(image, windowsSize, lowerBound, upperBound, wolfCoef);
      break;
    }
  }

  return binarized;
}

BinaryImage OutputGenerator::Processor::binarize(const QImage& image, const BinaryImage& mask) const {
  BinaryImage binarized = binarize(image);

  rasterOp<RopAnd<RopSrc, RopDst>>(binarized, mask);

  return binarized;
}

BinaryImage OutputGenerator::Processor::binarize(const QImage& image,
                                                 const QPolygonF& crop_area,
                                                 const BinaryImage* mask) const {
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
void OutputGenerator::Processor::maybeDespeckleInPlace(BinaryImage& image,
                                                       const QRect& image_rect,
                                                       const QRect& mask_rect,
                                                       double level,
                                                       BinaryImage* speckles_img,
                                                       const Dpi& dpi) const {
  const QRect src_rect(mask_rect.translated(-image_rect.topLeft()));
  const QRect dst_rect(mask_rect);

  if (speckles_img) {
    BinaryImage(m_outRect.size(), WHITE).swap(*speckles_img);
    if (!mask_rect.isEmpty()) {
      rasterOp<RopSrc>(*speckles_img, dst_rect, image, src_rect.topLeft());
    }
  }

  if (level != 0) {
    Despeckle::despeckleInPlace(image, dpi, level, m_status, m_dbg);

    if (m_dbg) {
      m_dbg->add(image, "despeckled");
    }
  }

  if (speckles_img) {
    if (!mask_rect.isEmpty()) {
      rasterOp<RopSubtract<RopDst, RopSrc>>(*speckles_img, dst_rect, image, src_rect.topLeft());
    }
  }

  m_status.throwIfCancelled();
}

double OutputGenerator::Processor::findSkew(const QImage& image) const {
  if (m_dewarpingOptions.needPostDeskew()
      && ((m_dewarpingOptions.dewarpingMode() == MARGINAL) || (m_dewarpingOptions.dewarpingMode() == MANUAL))) {
    const BinaryImage bw_image(image, BinaryThreshold::otsuThreshold(GrayscaleHistogram(image)));
    const Skew skew = SkewFinder().findSkew(bw_image);
    if ((skew.angle() != .0) && (skew.confidence() >= Skew::GOOD_CONFIDENCE)) {
      return skew.angle();
    }
  }
  return .0;
}

QImage OutputGenerator::Processor::segmentImage(const BinaryImage& image,
                                                const QImage& color_image,
                                                const BinaryImage* mask) const {
  const BlackWhiteOptions::ColorSegmenterOptions& segmenterOptions
      = m_colorParams.blackWhiteOptions().getColorSegmenterOptions();
  ColorSegmenter segmenter(m_dpi, segmenterOptions.getNoiseReduction(), segmenterOptions.getRedThresholdAdjustment(),
                           segmenterOptions.getGreenThresholdAdjustment(),
                           segmenterOptions.getBlueThresholdAdjustment());

  QImage segmented;
  {
    QImage color_image_content = color_image;
    if (mask) {
      applyMask(color_image_content, *mask);
    }
    if (!color_image.allGray()) {
      segmented = segmenter.segment(image, color_image_content);
    } else {
      segmented = segmenter.segment(image, GrayImage(color_image_content));
    }
  }

  if (m_dbg) {
    m_dbg->add(segmented, "segmented");
  }
  m_status.throwIfCancelled();

  return segmented;
}

QImage OutputGenerator::Processor::posterizeImage(const QImage& image, const QColor& background_color) const {
  const ColorCommonOptions::PosterizationOptions& posterizationOptions
      = m_colorParams.colorCommonOptions().getPosterizationOptions();
  Posterizer posterizer(posterizationOptions.getLevel(), posterizationOptions.isNormalizationEnabled(),
                        posterizationOptions.isForceBlackAndWhite(), 0, qRound(background_color.lightnessF() * 255));

  QImage posterized = posterizer.posterize(image);

  if (m_dbg) {
    m_dbg->add(posterized, "posterized");
  }
  m_status.throwIfCancelled();

  return posterized;
}

void OutputGenerator::Processor::processPictureZones(BinaryImage& mask,
                                                     ZoneSet& picture_zones,
                                                     const GrayImage& image) {
  if ((m_pictureShapeOptions.getPictureShape() != RECTANGULAR_SHAPE) || !m_outputProcessingParams.isAutoZonesFound()) {
    if (m_pictureShapeOptions.getPictureShape() != OFF_SHAPE) {
      mask = estimateBinarizationMask(image, m_workingBoundingRect, m_workingBoundingRect);
    }

    removeAutoPictureZones(picture_zones);
    m_settings->setPictureZones(m_pageId, picture_zones);
    m_outputProcessingParams.setAutoZonesFound(false);
    m_settings->setOutputProcessingParams(m_pageId, m_outputProcessingParams);
  }
  if ((m_pictureShapeOptions.getPictureShape() == RECTANGULAR_SHAPE) && !m_outputProcessingParams.isAutoZonesFound()) {
    std::vector<QRect> areas = findRectAreas(mask, WHITE, m_pictureShapeOptions.getSensitivity());

    const QTransform from_working_cs = [this]() {
      QTransform to_working_cs = m_xform.transform();
      to_working_cs *= QTransform().translate(-m_workingBoundingRect.x(), -m_workingBoundingRect.y());
      return to_working_cs.inverted();
    }();

    for (const QRect& area : areas) {
      picture_zones.add(createPictureZoneFromPoly(from_working_cs.map(QRectF(area))));
    }
    m_settings->setPictureZones(m_pageId, picture_zones);
    m_outputProcessingParams.setAutoZonesFound(true);
    m_settings->setOutputProcessingParams(m_pageId, m_outputProcessingParams);

    mask.fill(BLACK);
  }
}

ForegroundType OutputGenerator::Processor::getForegroundType() const {
  if (m_renderParams.needBinarization()) {
    if (m_renderParams.needColorSegmentation()) {
      return ForegroundType::INDEXED;
    } else {
      return ForegroundType::BINARY;
    }
  } else {
    return ForegroundType::COLOR;
  }
}

QImage OutputGenerator::Processor::transformToWorkingCs(bool normalize) const {
  QImage dst;
  if (normalize) {
    dst = normalizeIlluminationGray(m_inputGrayImage, m_preCropAreaInOriginalCs, m_xform.transform(),
                                    m_workingBoundingRect);
    if (m_colorOriginal) {
      assert(dst.format() == QImage::Format_Indexed8);
      QImage colorImg = transform(m_inputOrigImage, m_xform.transform(), m_workingBoundingRect,
                                  OutsidePixels::assumeColor(m_outsideBackgroundColor));
      adjustBrightnessGrayscale(colorImg, dst);
      dst = colorImg;
    }
  } else {
    if (!m_colorOriginal) {
      dst = transformToGray(m_inputGrayImage, m_xform.transform(), m_workingBoundingRect,
                            OutsidePixels::assumeColor(m_outsideBackgroundColor));
    } else {
      dst = transform(m_inputOrigImage, m_xform.transform(), m_workingBoundingRect,
                      OutsidePixels::assumeColor(m_outsideBackgroundColor));
    }
  }
  m_status.throwIfCancelled();

  return dst;
}

DistortionModel OutputGenerator::Processor::buildAutoDistortionModel(const GrayImage& warped_gray_output,
                                                                     const QTransform& to_original) const {
  DistortionModelBuilder model_builder(Vec2d(0, 1));

  TextLineTracer::trace(warped_gray_output, m_dpi, m_contentRectInWorkingCs, model_builder, m_status, m_dbg);
  model_builder.transform(to_original);

  TopBottomEdgeTracer::trace(m_inputGrayImage, model_builder.verticalBounds(), model_builder, m_status, m_dbg);

  DistortionModel distortion_model = model_builder.tryBuildModel(m_dbg, &m_inputGrayImage.toQImage());
  if (!distortion_model.isValid()) {
    setupTrivialDistortionModel(distortion_model);
  }

  BinaryImage bw_image(m_inputGrayImage, BinaryThreshold(64));

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

  QString stAngle;

  float max_angle = 2.75;

  if ((m_pageId.subPage() == PageId::SINGLE_PAGE) || (m_pageId.subPage() == PageId::LEFT_PAGE)) {
    float vert_skew_angle_left = vertBorderSkewAngle(top_polyline.front(), bottom_polyline.front());

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

        distortion_model.setBottomCurve(Curve(new_bottom_polyline));
      } else {
        std::vector<QPointF> new_top_polyline;

        QPointF pt(bottom_x, top_polyline.front().y());

        new_top_polyline.push_back(pt);

        for (auto i : top_polyline) {
          new_top_polyline.push_back(inv_transform.map(i));
        }

        distortion_model.setBottomCurve(Curve(new_top_polyline));
      }
    }
  } else {
    float vert_skew_angle_right = vertBorderSkewAngle(top_polyline.back(), bottom_polyline.back());

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

        distortion_model.setBottomCurve(Curve(new_bottom_polyline));
      } else {
        std::vector<QPointF> new_top_polyline;

        QPointF pt(bottom_x, top_polyline.back().y());

        for (auto i : top_polyline) {
          new_top_polyline.push_back(inv_transform.map(i));
        }

        new_top_polyline.push_back(pt);

        distortion_model.setBottomCurve(Curve(new_top_polyline));
      }
    }
  }

  return distortion_model;
}

DistortionModel OutputGenerator::Processor::buildMarginalDistortionModel() const {
  BinaryImage bw_image(m_inputGrayImage, BinaryThreshold(64));

  QTransform transform = m_xform.preRotation().transform(bw_image.size());
  QTransform inv_transform = transform.inverted();

  int degrees = m_xform.preRotation().toDegrees();
  bw_image = orthogonalRotation(bw_image, degrees);

  DistortionModel distortion_model;
  setupTrivialDistortionModel(distortion_model);

  int max_red_points = 5;
  XSpline top_spline;

  const std::vector<QPointF>& top_polyline = distortion_model.topCurve().polyline();

  const QLineF top_line(transform.map(top_polyline.front()), transform.map(top_polyline.back()));

  top_spline.appendControlPoint(top_line.p1(), 0);

  if ((m_pageId.subPage() == PageId::SINGLE_PAGE) || (m_pageId.subPage() == PageId::LEFT_PAGE)) {
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

  distortion_model.setTopCurve(Curve(top_spline));


  XSpline bottom_spline;

  const std::vector<QPointF>& bottom_polyline = distortion_model.bottomCurve().polyline();

  const QLineF bottom_line(transform.map(bottom_polyline.front()), transform.map(bottom_polyline.back()));

  bottom_spline.appendControlPoint(bottom_line.p1(), 0);

  if ((m_pageId.subPage() == PageId::SINGLE_PAGE) || (m_pageId.subPage() == PageId::LEFT_PAGE)) {
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

  distortion_model.setBottomCurve(Curve(bottom_spline));

  if (!distortion_model.isValid()) {
    setupTrivialDistortionModel(distortion_model);
  }

  if (m_dbg) {
    QImage out_image(bw_image.toQImage().convertToFormat(QImage::Format_RGB32));
    for (int i = 0; i <= top_spline.numSegments(); i++) {
      drawPoint(out_image, top_spline.controlPointPosition(i));
    }
    for (int i = 0; i <= bottom_spline.numSegments(); i++) {
      drawPoint(out_image, bottom_spline.controlPointPosition(i));
    }
    m_dbg->add(out_image, "marginal dewarping");
  }

  return distortion_model;
}
}  // namespace output