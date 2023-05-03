// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

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
#include <core/ApplicationSettings.h>
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
#include <QPainterPath>
#include <QPointF>
#include <QPolygonF>
#include <QSize>
#include <QTransform>
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
OutputGenerator::OutputGenerator(const ImageTransformation& xform, const QPolygonF& contentRectPhys)
    : m_xform(xform),
      m_outRect(xform.resultingRect().toRect()),
      m_contentRect(xform.transform().map(contentRectPhys).boundingRect().toRect()) {
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

class OutputGenerator::Processor {
 public:
  Processor(const OutputGenerator& generator,
            const PageId& pageId,
            const std::shared_ptr<Settings>& settings,
            const FilterData& input,
            const TaskStatus& status,
            DebugImages* dbg);

  std::unique_ptr<OutputImage> process(ZoneSet& pictureZones,
                                       const ZoneSet& fillZones,
                                       DistortionModel& distortionModel,
                                       const DepthPerception& depthPerception,
                                       BinaryImage* autoPictureMask,
                                       BinaryImage* specklesImage);

 private:
  void initParams();

  void calcAreas();

  void updateBlackOnWhite(const FilterData& input);

  void initFilterData(const FilterData& input);

  std::unique_ptr<OutputImage> processImpl(ZoneSet& pictureZones,
                                           const ZoneSet& fillZones,
                                           DistortionModel& distortionModel,
                                           const DepthPerception& depthPerception,
                                           BinaryImage* autoPictureMask,
                                           BinaryImage* specklesImage);

  std::unique_ptr<OutputImage> processWithoutDewarping(ZoneSet& pictureZones,
                                                       const ZoneSet& fillZones,
                                                       BinaryImage* autoPictureMask,
                                                       BinaryImage* specklesImage);

  std::unique_ptr<OutputImage> processWithDewarping(ZoneSet& pictureZones,
                                                    const ZoneSet& fillZones,
                                                    DistortionModel& distortionModel,
                                                    const DepthPerception& depthPerception,
                                                    BinaryImage* autoPictureMask,
                                                    BinaryImage* specklesImage);

  std::unique_ptr<OutputImage> buildEmptyImage() const;

  double findSkew(const QImage& image) const;

  void setupTrivialDistortionModel(DistortionModel& distortionModel) const;

  static CylindricalSurfaceDewarper createDewarper(const DistortionModel& distortionModel,
                                                   const QTransform& distortionModelToTarget,
                                                   double depthPerception);

  QImage dewarp(const QTransform& origToSrc,
                const QImage& src,
                const QTransform& srcToOutput,
                const DistortionModel& distortionModel,
                const DepthPerception& depthPerception,
                const QColor& bgColor) const;

  GrayImage normalizeIlluminationGray(const QImage& input,
                                      const QPolygonF& areaToConsider,
                                      const QTransform& xform,
                                      const QRect& targetRect,
                                      GrayImage* background = nullptr) const;

  GrayImage detectPictures(const GrayImage& input300dpi) const;

  BinaryImage estimateBinarizationMask(const GrayImage& graySource,
                                       const QRect& sourceRect,
                                       const QRect& sourceSubRect) const;

  void modifyBinarizationMask(BinaryImage& bwMask, const QRect& maskRect, const ZoneSet& zones) const;

  BinaryThreshold adjustThreshold(BinaryThreshold threshold) const;

  BinaryThreshold calcBinarizationThreshold(const QImage& image, const BinaryImage& mask) const;

  BinaryThreshold calcBinarizationThreshold(const QImage& image,
                                            const QPolygonF& cropArea,
                                            const BinaryImage* mask = nullptr) const;

  void morphologicalSmoothInPlace(BinaryImage& binImg) const;

  BinaryImage binarize(const QImage& image) const;

  BinaryImage binarize(const QImage& image, const BinaryImage& mask) const;

  BinaryImage binarize(const QImage& image, const QPolygonF& cropArea, const BinaryImage* mask = nullptr) const;

  void maybeDespeckleInPlace(BinaryImage& image,
                             const QRect& imageRect,
                             const QRect& maskRect,
                             double level,
                             BinaryImage* specklesImg,
                             const Dpi& dpi) const;

  QImage segmentImage(const BinaryImage& image, const QImage& colorImage) const;

  QImage posterizeImage(const QImage& image, const QColor& backgroundColor = Qt::white) const;

  ForegroundType getForegroundType() const;

  void processPictureZones(BinaryImage& mask, ZoneSet& pictureZones, const GrayImage& image);

  QImage transformToWorkingCs(bool normalize) const;

  DistortionModel buildAutoDistortionModel(const GrayImage& warpedGrayOutput, const QTransform& toOriginal) const;

  DistortionModel buildMarginalDistortionModel() const;

  const ImageTransformation& m_xform;
  const QRect& m_outRect;
  const QRect& m_contentRect;

  const PageId m_pageId;
  const std::shared_ptr<Settings> m_settings;

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
                                                      ZoneSet& pictureZones,
                                                      const ZoneSet& fillZones,
                                                      DistortionModel& distortionModel,
                                                      const DepthPerception& depthPerception,
                                                      BinaryImage* autoPictureMask,
                                                      BinaryImage* specklesImage,
                                                      DebugImages* dbg,
                                                      const PageId& pageId,
                                                      const std::shared_ptr<Settings>& settings) const {
  return Processor(*this, pageId, settings, input, status, dbg)
      .process(pictureZones, fillZones, distortionModel, depthPerception, autoPictureMask, specklesImage);
}

OutputGenerator::Processor::Processor(const OutputGenerator& generator,
                                      const PageId& pageId,
                                      const std::shared_ptr<Settings>& settings,
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

std::unique_ptr<OutputImage> OutputGenerator::Processor::process(ZoneSet& pictureZones,
                                                                 const ZoneSet& fillZones,
                                                                 DistortionModel& distortionModel,
                                                                 const DepthPerception& depthPerception,
                                                                 BinaryImage* autoPictureMask,
                                                                 BinaryImage* specklesImage) {
  std::unique_ptr<OutputImage> image
      = processImpl(pictureZones, fillZones, distortionModel, depthPerception, autoPictureMask, specklesImage);
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
  m_croppedContentArea = PolygonUtils::round(m_croppedContentArea.intersected(QRectF(m_croppedContentRect)));
  if (m_croppedContentRect.isEmpty()) {
    m_blank = true;
    return;
  }

  m_outCropArea = m_preCropArea.intersected(QRectF(m_outRect));

  // For various reasons, we need some whitespace around the content
  // area.  This is the number of pixels of such whitespace.
  const int contentMargin = m_dpi.vertical() * 20 / 300;
  // The content area (in output image coordinates) extended
  // with contentMargin.  Note that we prevent that extension
  // from reaching the neighboring page.
  // This is the area we are going to pass to estimateBackground().
  // estimateBackground() needs some margins around content, and
  // generally smaller margins are better, except when there is
  // some garbage that connects the content to the edge of the
  // image area.
  m_workingBoundingRect = m_preCropArea
                              .intersected(QRectF(m_croppedContentRect.adjusted(-contentMargin, -contentMargin,
                                                                                contentMargin, contentMargin)))
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

  auto* imageLine = reinterpret_cast<PixelType*>(img.bits());
  const int imageStride = img.bytesPerLine() / sizeof(PixelType);

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      imageLine[x] = reserveBlackAndWhite<PixelType>(imageLine[x]);
    }
    imageLine += imageStride;
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

  auto* imageLine = reinterpret_cast<PixelType*>(img.bits());
  const int imageStride = img.bytesPerLine() / sizeof(PixelType);
  const uint32_t* maskLine = mask.data();
  const int maskStride = mask.wordsPerLine();
  const uint32_t msb = uint32_t(1) << 31;

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      if (maskLine[x >> 5] & (msb >> (x & 31))) {
        imageLine[x] = reserveBlackAndWhite<PixelType>(imageLine[x]);
      }
    }
    imageLine += imageStride;
    maskLine += maskStride;
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
void fillExcept(QImage& image, const BinaryImage& bwMask, const QColor& color) {
  auto* imageLine = reinterpret_cast<MixedPixel*>(image.bits());
  const int imageStride = image.bytesPerLine() / sizeof(MixedPixel);
  const uint32_t* bwMaskLine = bwMask.data();
  const int bwMaskStride = bwMask.wordsPerLine();
  const int width = image.width();
  const int height = image.height();
  const uint32_t msb = uint32_t(1) << 31;
  const auto fillingPixel = static_cast<MixedPixel>(color.rgba());

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      if (!(bwMaskLine[x >> 5] & (msb >> (x & 31)))) {
        imageLine[x] = fillingPixel;
      }
    }
    imageLine += imageStride;
    bwMaskLine += bwMaskStride;
  }
}

void fillExcept(BinaryImage& image, const BinaryImage& bwMask, const BWColor color) {
  uint32_t* imageLine = image.data();
  const int imageStride = image.wordsPerLine();
  const uint32_t* bwMaskLine = bwMask.data();
  const int bwMaskStride = bwMask.wordsPerLine();
  const int width = image.width();
  const int height = image.height();
  const uint32_t msb = uint32_t(1) << 31;

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      if (!(bwMaskLine[x >> 5] & (msb >> (x & 31)))) {
        if (color == BLACK) {
          imageLine[x >> 5] |= (msb >> (x & 31));
        } else {
          imageLine[x >> 5] &= ~(msb >> (x & 31));
        }
      }
    }
    imageLine += imageStride;
    bwMaskLine += bwMaskStride;
  }
}

void fillMarginsInPlace(QImage& image,
                        const QPolygonF& contentPoly,
                        const QColor& color,
                        const bool antialiasing = true) {
  if (contentPoly.intersected(QRectF(image.rect())) != contentPoly) {
    throw std::invalid_argument("fillMarginsInPlace: the content area exceeds image rect.");
  }

  if ((image.format() == QImage::Format_Mono) || (image.format() == QImage::Format_MonoLSB)) {
    BinaryImage binaryImage(image);
    PolygonRasterizer::fillExcept(binaryImage, (color == Qt::black) ? BLACK : WHITE, contentPoly, Qt::WindingFill);
    image = binaryImage.toQImage();
    return;
  }
  if ((image.format() == QImage::Format_Indexed8) && image.isGrayscale()) {
    PolygonRasterizer::grayFillExcept(image, static_cast<unsigned char>(qGray(color.rgb())), contentPoly,
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

    QPainterPath outerPath;
    outerPath.addRect(image.rect());
    QPainterPath innerPath;
    innerPath.addPolygon(PolygonUtils::round(contentPoly));

    painter.drawPath(outerPath.subtracted(innerPath));
  }
  image = image.convertToFormat(imageFormat);
}

void fillMarginsInPlace(BinaryImage& image, const QPolygonF& contentPoly, const BWColor color) {
  if (contentPoly.intersected(QRectF(image.rect())) != contentPoly) {
    throw std::invalid_argument("fillMarginsInPlace: the content area exceeds image rect.");
  }

  PolygonRasterizer::fillExcept(image, color, contentPoly, Qt::WindingFill);
}

void fillMarginsInPlace(BinaryImage& image, const BinaryImage& contentMask, const BWColor color) {
  if (image.size() != contentMask.size()) {
    throw std::invalid_argument("fillMarginsInPlace: img and mask have different sizes");
  }

  fillExcept(image, contentMask, color);
}

void fillMarginsInPlace(QImage& image, const BinaryImage& contentMask, const QColor& color) {
  if (image.size() != contentMask.size()) {
    throw std::invalid_argument("fillMarginsInPlace: img and mask have different sizes");
  }

  if ((image.format() == QImage::Format_Mono) || (image.format() == QImage::Format_MonoLSB)) {
    BinaryImage binaryImage(image);
    fillExcept(binaryImage, contentMask, (color == Qt::black) ? BLACK : WHITE);
    image = binaryImage.toQImage();
    return;
  }

  if ((image.format() == QImage::Format_Indexed8) && image.isGrayscale()) {
    fillExcept<uint8_t>(image, contentMask, color);
  } else {
    assert(image.format() == QImage::Format_RGB32 || image.format() == QImage::Format_ARGB32);
    fillExcept<uint32_t>(image, contentMask, color);
  }
}

void removeAutoPictureZones(ZoneSet& pictureZones) {
  for (auto it = pictureZones.begin(); it != pictureZones.end();) {
    const Zone& zone = *it;
    if (zone.properties().locateOrDefault<ZoneCategoryProperty>()->zoneCategory() == ZoneCategoryProperty::AUTO) {
      it = pictureZones.erase(it);
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
                           const boost::function<QPointF(const QPointF&)>& origToOutput,
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
      const QPolygonF poly(zone.spline().transformed(origToOutput).toPolygon());
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
                           const boost::function<QPointF(const QPointF&)>& origToOutput) {
  if (zones.empty()) {
    return;
  }

  for (const Zone& zone : zones) {
    const QColor color(zone.properties().locateOrDefault<FillColorProperty>()->color());
    const BWColor bwColor = qGray(color.rgb()) < 128 ? BLACK : WHITE;
    const QPolygonF poly(zone.spline().transformed(origToOutput).toPolygon());
    PolygonRasterizer::fill(img, bwColor, poly, Qt::WindingFill);
  }
}

void applyFillZonesInPlace(BinaryImage& img, const ZoneSet& zones, const QTransform& transform) {
  applyFillZonesInPlace(img, zones, boost::bind(static_cast<MapPointFunc>(&QTransform::map), transform, _1));
}

void applyFillZonesToMixedInPlace(QImage& img,
                                  const ZoneSet& zones,
                                  const boost::function<QPointF(const QPointF&)>& origToOutput,
                                  const BinaryImage& pictureMask,
                                  bool binaryMode) {
  if (binaryMode) {
    BinaryImage bwContent(img, BinaryThreshold(1));
    applyFillZonesInPlace(bwContent, zones, origToOutput);
    applyFillZonesInPlace(img, zones, origToOutput);
    combineImages(img, bwContent, pictureMask);
  } else {
    QImage content(img);
    applyMask(content, pictureMask);
    applyFillZonesInPlace(content, zones, origToOutput, false);
    applyFillZonesInPlace(img, zones, origToOutput);
    combineImages(img, content, pictureMask);
  }
}

void applyFillZonesToMixedInPlace(QImage& img,
                                  const ZoneSet& zones,
                                  const QTransform& transform,
                                  const BinaryImage& pictureMask,
                                  bool binaryMode) {
  applyFillZonesToMixedInPlace(img, zones, boost::bind(static_cast<MapPointFunc>(&QTransform::map), transform, _1),
                               pictureMask, binaryMode);
}

void applyFillZonesToMask(BinaryImage& mask,
                          const ZoneSet& zones,
                          const boost::function<QPointF(const QPointF&)>& origToOutput,
                          const BWColor fillColor = BLACK) {
  if (zones.empty()) {
    return;
  }

  for (const Zone& zone : zones) {
    const QPolygonF poly(zone.spline().transformed(origToOutput).toPolygon());
    PolygonRasterizer::fill(mask, fillColor, poly, Qt::WindingFill);
  }
}

void applyFillZonesToMask(BinaryImage& mask,
                          const ZoneSet& zones,
                          const QTransform& transform,
                          const BWColor fillColor = BLACK) {
  applyFillZonesToMask(mask, zones, boost::bind((MapPointFunc) &QTransform::map, transform, _1), fillColor);
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

std::vector<QRect> findRectAreas(const BinaryImage& mask, BWColor contentColor, int sensitivity) {
  if (mask.isNull()) {
    return {};
  }

  std::vector<QRect> areas;

  const int w = mask.width();
  const int h = mask.height();
  const int wpl = mask.wordsPerLine();
  const int lastWordIdx = (w - 1) >> 5;
  const int lastWordBits = w - (lastWordIdx << 5);
  const int lastWordUnusedBits = 32 - lastWordBits;
  const uint32_t lastWordMask = ~uint32_t(0) << lastWordUnusedBits;
  const uint32_t modifier = (contentColor == WHITE) ? ~uint32_t(0) : 0;
  const uint32_t* const data = mask.data();

  const uint32_t* line = data;
  // create list of filled continuous blocks on each line
  for (int y = 0; y < h; ++y, line += wpl) {
    QRect area;
    area.setTop(y);
    area.setBottom(y);
    bool areaFound = false;
    for (int i = 0; i <= lastWordIdx; ++i) {
      uint32_t word = line[i] ^ modifier;
      if (i == lastWordIdx) {
        // The last (possibly incomplete) word.
        word &= lastWordMask;
      }
      if (word) {
        if (!areaFound) {
          area.setLeft((i << 5) + 31 - findPositionOfTheHighestBitSet(~line[i]));
          areaFound = true;
        }
        area.setRight(((i + 1) << 5) - 1);
      } else {
        if (areaFound) {
          uint32_t v = line[i - 1];
          if (v) {
            area.setRight(area.right() - countConsecutiveZeroBitsTrailing(~v));
          }
          areas.emplace_back(area);
          areaFound = false;
        }
      }
    }
    if (areaFound) {
      uint32_t v = line[lastWordIdx];
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
      int wordWidth = area.width() >> 5;

      int left = area.left();
      int leftWord = left >> 5;
      int right = area.x() + area.width();
      int rightWord = right >> 5;
      int top = area.top();
      int bottom = area.bottom();

      const uint32_t* pdata = mask.data();

      const auto criterium = (int) (area.width() * percent);
      const auto criteriumWord = (int) (wordWidth * percent);

      // cut the dirty upper lines
      for (int y = top; y < bottom; y++) {
        line = pdata + wpl * y;

        int mword = 0;

        for (int k = leftWord; k < rightWord; k++) {
          if (!line[k]) {
            mword++;  // count the totally white words
          }
        }

        if (mword > criteriumWord) {
          area.setTop(y);
          break;
        }
      }

      // cut the dirty bottom lines
      for (int y = bottom; y > top; y--) {
        line = pdata + wpl * y;

        int mword = 0;

        for (int k = leftWord; k < rightWord; k++) {
          if (!line[k]) {
            mword++;
          }
        }

        if (mword > criteriumWord) {
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

void applyAffineTransform(QImage& image, const QTransform& xform, const QColor& outsideColor) {
  if (xform.isIdentity()) {
    return;
  }
  image = transform(image, xform, image.rect(), OutsidePixels::assumeWeakColor(outsideColor));
}

void applyAffineTransform(BinaryImage& image, const QTransform& xform, const BWColor outsideColor) {
  if (xform.isIdentity()) {
    return;
  }
  const QColor color = (outsideColor == BLACK) ? Qt::black : Qt::white;
  QImage converted = image.toQImage();
  applyAffineTransform(converted, xform, color);
  image = BinaryImage(converted);
}

void hitMissReplaceAllDirections(BinaryImage& img,
                                 const char* const pattern,
                                 const int patternWidth,
                                 const int patternHeight) {
  hitMissReplaceInPlace(img, WHITE, pattern, patternWidth, patternHeight);

  std::vector<char> patternData(static_cast<unsigned long long int>(patternWidth * patternHeight), ' ');
  char* const newPattern = &patternData[0];

  // Rotate 90 degrees clockwise.
  const char* p = pattern;
  int newWidth = patternHeight;
  int newHeight = patternWidth;
  for (int y = 0; y < patternHeight; ++y) {
    for (int x = 0; x < patternWidth; ++x, ++p) {
      const int newX = patternHeight - 1 - y;
      const int newY = x;
      newPattern[newY * newWidth + newX] = *p;
    }
  }
  hitMissReplaceInPlace(img, WHITE, newPattern, newWidth, newHeight);

  // Rotate upside down.
  p = pattern;
  newWidth = patternWidth;
  newHeight = patternHeight;
  for (int y = 0; y < patternHeight; ++y) {
    for (int x = 0; x < patternWidth; ++x, ++p) {
      const int newX = patternWidth - 1 - x;
      const int newY = patternHeight - 1 - y;
      newPattern[newY * newWidth + newX] = *p;
    }
  }
  hitMissReplaceInPlace(img, WHITE, newPattern, newWidth, newHeight);
  // Rotate 90 degrees counter-clockwise.
  p = pattern;
  newWidth = patternHeight;
  newHeight = patternWidth;
  for (int y = 0; y < patternHeight; ++y) {
    for (int x = 0; x < patternWidth; ++x, ++p) {
      const int newX = y;
      const int newY = patternWidth - 1 - x;
      newPattern[newY * newWidth + newX] = *p;
    }
  }
  hitMissReplaceInPlace(img, WHITE, newPattern, newWidth, newHeight);
}

QSize calcLocalWindowSize(const Dpi& dpi) {
  const QSizeF sizeMm(3, 30);
  const QSizeF sizeInch(sizeMm * constants::MM2INCH);
  const QSizeF sizePixelsF(dpi.horizontal() * sizeInch.width(), dpi.vertical() * sizeInch.height());
  QSize sizePixels(sizePixelsF.toSize());

  if (sizePixels.width() < 3) {
    sizePixels.setWidth(3);
  }
  if (sizePixels.height() < 3) {
    sizePixels.setHeight(3);
  }
  return sizePixels;
}

void movePointToTopMargin(BinaryImage& bwImage, XSpline& spline, int idx) {
  QPointF pos = spline.controlPointPosition(idx);

  for (int j = 0; j < pos.y(); j++) {
    if (bwImage.getPixel(static_cast<int>(pos.x()), j) == WHITE) {
      int count = 0;
      int checkNum = 16;

      for (int jj = j; jj < (j + checkNum); jj++) {
        if (bwImage.getPixel(static_cast<int>(pos.x()), jj) == WHITE) {
          count++;
        }
      }

      if (count == checkNum) {
        pos.setY(j);
        spline.moveControlPoint(idx, pos);
        break;
      }
    }
  }
}

void movePointToBottomMargin(BinaryImage& bwImage, XSpline& spline, int idx) {
  QPointF pos = spline.controlPointPosition(idx);

  for (int j = bwImage.height() - 1; j > pos.y(); j--) {
    if (bwImage.getPixel(static_cast<int>(pos.x()), j) == WHITE) {
      int count = 0;
      int checkNum = 16;

      for (int jj = j; jj > (j - checkNum); jj--) {
        if (bwImage.getPixel(static_cast<int>(pos.x()), jj) == WHITE) {
          count++;
        }
      }

      if (count == checkNum) {
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

void movePointToTopMargin(BinaryImage& bwImage, std::vector<QPointF>& polyline, int idx) {
  QPointF& pos = polyline[idx];

  for (int j = 0; j < pos.y(); j++) {
    if (bwImage.getPixel(static_cast<int>(pos.x()), j) == WHITE) {
      int count = 0;
      int checkNum = 16;

      for (int jj = j; jj < (j + checkNum); jj++) {
        if (bwImage.getPixel(static_cast<int>(pos.x()), jj) == WHITE) {
          count++;
        }
      }

      if (count == checkNum) {
        pos.setY(j);

        break;
      }
    }
  }
}

void movePointToBottomMargin(BinaryImage& bwImage, std::vector<QPointF>& polyline, int idx) {
  QPointF& pos = polyline[idx];

  for (int j = bwImage.height() - 1; j > pos.y(); j--) {
    if (bwImage.getPixel(static_cast<int>(pos.x()), j) == WHITE) {
      int count = 0;
      int checkNum = 16;

      for (int jj = j; jj > (j - checkNum); jj--) {
        if (bwImage.getPixel(static_cast<int>(pos.x()), jj) == WHITE) {
          count++;
        }
      }

      if (count == checkNum) {
        pos.setY(j);

        break;
      }
    }
  }
}

inline float vertBorderSkewAngle(const QPointF& top, const QPointF& bottom) {
  return static_cast<float>(
      std::abs(std::atan((bottom.x() - top.x()) / (bottom.y() - top.y())) * 180.0 / constants::PI));
}

QImage smoothToGrayscale(const QImage& src, const Dpi& dpi) {
  const int minDpi = std::min(dpi.horizontal(), dpi.vertical());
  int window;
  int degree;
  if (minDpi <= 200) {
    window = 5;
    degree = 3;
  } else if (minDpi <= 400) {
    window = 7;
    degree = 4;
  } else if (minDpi <= 800) {
    window = 11;
    degree = 4;
  } else {
    window = 11;
    degree = 2;
  }
  return savGolFilter(src, QSize(window, window), degree, degree);
}

QSize from300dpi(const QSize& size, const Dpi& targetDpi) {
  const double hscale = targetDpi.horizontal() / 300.0;
  const double vscale = targetDpi.vertical() / 300.0;
  const int width = qRound(size.width() * hscale);
  const int height = qRound(size.height() * vscale);
  return QSize(std::max(1, width), std::max(1, height));
}

QSize to300dpi(const QSize& size, const Dpi& sourceDpi) {
  const double hscale = 300.0 / sourceDpi.horizontal();
  const double vscale = 300.0 / sourceDpi.vertical();
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
  ApplicationSettings& appSettings = ApplicationSettings::getInstance();
  Params params = m_settings->getParams(m_pageId);
  if ((appSettings.isBlackOnWhiteDetectionEnabled() && appSettings.isBlackOnWhiteDetectionOutputEnabled())
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

std::unique_ptr<OutputImage> OutputGenerator::Processor::processImpl(ZoneSet& pictureZones,
                                                                     const ZoneSet& fillZones,
                                                                     DistortionModel& distortionModel,
                                                                     const DepthPerception& depthPerception,
                                                                     BinaryImage* autoPictureMask,
                                                                     BinaryImage* specklesImage) {
  if (m_blank) {
    return buildEmptyImage();
  }

  m_outsideBackgroundColor = BackgroundColorCalculator::calcDominantBackgroundColor(
      m_colorOriginal ? m_inputOrigImage : m_inputGrayImage, m_outCropAreaInOriginalCs);

  if ((m_dewarpingOptions.dewarpingMode() == AUTO) || (m_dewarpingOptions.dewarpingMode() == MARGINAL)
      || ((m_dewarpingOptions.dewarpingMode() == MANUAL) && distortionModel.isValid())) {
    return processWithDewarping(pictureZones, fillZones, distortionModel, depthPerception, autoPictureMask,
                                specklesImage);
  } else {
    return processWithoutDewarping(pictureZones, fillZones, autoPictureMask, specklesImage);
  }
}

std::unique_ptr<OutputImage> OutputGenerator::Processor::processWithoutDewarping(ZoneSet& pictureZones,
                                                                                 const ZoneSet& fillZones,
                                                                                 BinaryImage* autoPictureMask,
                                                                                 BinaryImage* specklesImage) {
  OutputImageBuilder imageBuilder;

  QImage maybeNormalized = transformToWorkingCs(m_renderParams.normalizeIllumination());
  if (m_dbg) {
    m_dbg->add(maybeNormalized, "maybeNormalized");
  }

  if (m_renderParams.normalizeIllumination()) {
    m_outsideBackgroundColor
        = BackgroundColorCalculator::calcDominantBackgroundColor(maybeNormalized, m_outCropAreaInWorkingCs);
  }

  m_status.throwIfCancelled();

  if (m_renderParams.binaryOutput()) {
    QImage maybeSmoothed;
    // We only do smoothing if we are going to do binarization later.
    if (!m_renderParams.needSavitzkyGolaySmoothing()) {
      maybeSmoothed = maybeNormalized;
    } else {
      maybeSmoothed = smoothToGrayscale(maybeNormalized, m_dpi);
      if (m_dbg) {
        m_dbg->add(maybeSmoothed, "smoothed");
      }
      m_status.throwIfCancelled();
    }

    // don't destroy as it's needed for color segmentation
    if (!m_renderParams.needColorSegmentation()) {
      maybeNormalized = QImage();
    }

    BinaryImage bwContent = binarize(maybeSmoothed, m_contentAreaInWorkingCs);
    maybeSmoothed = QImage();  // Save memory.
    if (m_dbg) {
      m_dbg->add(bwContent, "binarized_and_cropped");
    }
    m_status.throwIfCancelled();

    if (m_renderParams.needMorphologicalSmoothing()) {
      morphologicalSmoothInPlace(bwContent);
      m_status.throwIfCancelled();
    }

    BinaryImage dst(m_targetSize, WHITE);
    rasterOp<RopSrc>(dst, m_croppedContentRect, bwContent, m_contentRectInWorkingCs.topLeft());
    bwContent.release();  // Save memory.

    // It's important to keep despeckling the very last operation
    // affecting the binary part of the output. That's because
    // we will be reconstructing the input to this despeckling
    // operation from the final output file.
    maybeDespeckleInPlace(dst, m_outRect, m_outRect, m_despeckleLevel, specklesImage, m_dpi);

    if (!m_renderParams.needColorSegmentation()) {
      if (!m_blackOnWhite) {
        dst.invert();
      }

      applyFillZonesInPlace(dst, fillZones, m_xform.transform());

      imageBuilder.setImage(dst.toQImage());
    } else {
      QImage segmentedImage;
      {
        QImage colorImage(m_targetSize, maybeNormalized.format());
        if (maybeNormalized.format() == QImage::Format_Indexed8) {
          colorImage.setColorTable(maybeNormalized.colorTable());
        }
        colorImage.fill(Qt::white);
        drawOver(colorImage, m_croppedContentRect, maybeNormalized, m_contentRectInWorkingCs);
        maybeNormalized = QImage();

        segmentedImage = segmentImage(dst, colorImage);
        dst.release();
      }

      if (m_renderParams.posterize()) {
        segmentedImage = posterizeImage(segmentedImage, m_outsideBackgroundColor);
      }

      if (!m_blackOnWhite) {
        segmentedImage.invertPixels();
      }

      applyFillZonesInPlace(segmentedImage, fillZones, m_xform.transform(), false);
      if (m_dbg) {
        m_dbg->add(segmentedImage, "segmented_with_fill_zones");
      }
      m_status.throwIfCancelled();

      imageBuilder.setImage(segmentedImage);
    }
    return imageBuilder.build();
  }

  BinaryImage bwContentMaskOutput;
  BinaryImage bwContentOutput;
  if (m_renderParams.mixedOutput()) {
    BinaryImage bwMask(m_workingBoundingRect.size(), BLACK);
    processPictureZones(bwMask, pictureZones, GrayImage(maybeNormalized));
    if (m_dbg) {
      m_dbg->add(bwMask, "bwMask");
    }
    m_status.throwIfCancelled();

    if (autoPictureMask) {
      if (autoPictureMask->size() != m_targetSize) {
        BinaryImage(m_targetSize).swap(*autoPictureMask);
      }
      autoPictureMask->fill(BLACK);
      rasterOp<RopSrc>(*autoPictureMask, m_croppedContentRect, bwMask, m_contentRectInWorkingCs.topLeft());
    }
    m_status.throwIfCancelled();

    modifyBinarizationMask(bwMask, m_workingBoundingRect, pictureZones);
    fillMarginsInPlace(bwMask, m_contentAreaInWorkingCs, BLACK);
    if (m_dbg) {
      m_dbg->add(bwMask, "bwMask with zones");
    }
    m_status.throwIfCancelled();

    if (m_renderParams.needBinarization()) {
      QImage maybeSmoothed;
      if (!m_renderParams.needSavitzkyGolaySmoothing()) {
        maybeSmoothed = maybeNormalized;
      } else {
        maybeSmoothed = smoothToGrayscale(maybeNormalized, m_dpi);
        if (m_dbg) {
          m_dbg->add(maybeSmoothed, "smoothed");
        }
        m_status.throwIfCancelled();
      }

      BinaryImage bwMaskFilled(bwMask);
      fillMarginsInPlace(bwMaskFilled, m_contentAreaInWorkingCs, WHITE);
      BinaryImage bwContent = binarize(maybeSmoothed, bwMaskFilled);
      bwMaskFilled.release();
      maybeSmoothed = QImage();  // Save memory.
      if (m_dbg) {
        m_dbg->add(bwContent, "binarized_and_cropped");
      }
      m_status.throwIfCancelled();

      if (m_renderParams.needMorphologicalSmoothing()) {
        morphologicalSmoothInPlace(bwContent);
      }

      // It's important to keep despeckling the very last operation
      // affecting the binary part of the output. That's because
      // we will be reconstructing the input to this despeckling
      // operation from the final output file.
      maybeDespeckleInPlace(bwContent, m_workingBoundingRect, m_croppedContentRect, m_despeckleLevel, specklesImage,
                            m_dpi);

      if (!m_renderParams.normalizeIlluminationColor()) {
        m_outsideBackgroundColor = BackgroundColorCalculator::calcDominantBackgroundColor(
            m_colorOriginal ? m_inputOrigImage : m_inputGrayImage, m_outCropAreaInOriginalCs);
        maybeNormalized = transformToWorkingCs(false);
      }

      if (!m_renderParams.needColorSegmentation()) {
        if (m_renderParams.originalBackground()) {
          combineImages(maybeNormalized, bwContent);
        } else {
          combineImages(maybeNormalized, bwContent, bwMask);
        }
      } else {
        QImage segmentedImage = segmentImage(bwContent, maybeNormalized);
        if (m_renderParams.posterize()) {
          segmentedImage = posterizeImage(segmentedImage, m_outsideBackgroundColor);
        }

        if (m_renderParams.originalBackground()) {
          combineImages(maybeNormalized, segmentedImage);
        } else {
          combineImages(maybeNormalized, segmentedImage, bwMask);
        }
      }

      if (m_dbg) {
        m_dbg->add(maybeNormalized, "combined");
      }

      if (m_renderParams.originalBackground()) {
        reserveBlackAndWhite(maybeNormalized, bwContent.inverted());
      } else {
        reserveBlackAndWhite(maybeNormalized, bwMask.inverted());
      }
      m_status.throwIfCancelled();

      if (m_renderParams.originalBackground()) {
        bwContentOutput = BinaryImage(m_targetSize, WHITE);
        rasterOp<RopSrc>(bwContentOutput, m_croppedContentRect, bwContent, m_contentRectInWorkingCs.topLeft());
      }
    }

    bwContentMaskOutput = BinaryImage(m_targetSize, BLACK);
    rasterOp<RopSrc>(bwContentMaskOutput, m_croppedContentRect, bwMask, m_contentRectInWorkingCs.topLeft());
  }

  assert(!m_targetSize.isEmpty());
  QImage dst(m_targetSize, maybeNormalized.format());
  if (maybeNormalized.format() == QImage::Format_Indexed8) {
    dst.setColorTable(maybeNormalized.colorTable());
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
  fillMarginsInPlace(maybeNormalized, m_contentAreaInWorkingCs, m_outsideBackgroundColor);
  dst.fill(m_outsideBackgroundColor);

  drawOver(dst, m_croppedContentRect, maybeNormalized, m_contentRectInWorkingCs);
  maybeNormalized = QImage();

  if (!m_blackOnWhite) {
    dst.invertPixels();
  }

  if (m_renderParams.mixedOutput() && m_renderParams.needBinarization()) {
    applyFillZonesToMixedInPlace(dst, fillZones, m_xform.transform(), bwContentMaskOutput,
                                 !m_renderParams.needColorSegmentation());
  } else {
    applyFillZonesInPlace(dst, fillZones, m_xform.transform());
  }
  if (m_renderParams.originalBackground()) {
    applyFillZonesToMask(bwContentOutput, fillZones, m_xform.transform());
    rasterOp<RopAnd<RopSrc, RopDst>>(bwContentOutput, bwContentMaskOutput);
  }
  if (m_dbg) {
    m_dbg->add(dst, "fillZones");
  }
  m_status.throwIfCancelled();

  if (m_renderParams.posterize() && !m_renderParams.needBinarization()) {
    dst = posterizeImage(dst);
  }

  if (m_renderParams.splitOutput()) {
    imageBuilder.setForegroundType(getForegroundType());
    if (!m_renderParams.originalBackground()) {
      imageBuilder.setForegroundMask(bwContentMaskOutput);
    } else {
      imageBuilder.setForegroundMask(bwContentOutput);
      imageBuilder.setBackgroundMask(bwContentMaskOutput.inverted());
    }
  }
  return imageBuilder.setImage(dst).build();
}

std::unique_ptr<OutputImage> OutputGenerator::Processor::processWithDewarping(ZoneSet& pictureZones,
                                                                              const ZoneSet& fillZones,
                                                                              DistortionModel& distortionModel,
                                                                              const DepthPerception& depthPerception,
                                                                              BinaryImage* autoPictureMask,
                                                                              BinaryImage* specklesImage) {
  OutputImageBuilder imageBuilder;

  // The output we would get if dewarping was turned off, except always grayscale.
  // Used for automatic picture detection and binarization threshold calculation.
  // This image corresponds to the area of workingBoundingRect.
  GrayImage warpedGrayOutput;
  if (!m_renderParams.normalizeIllumination()) {
    warpedGrayOutput = transformToGray(m_inputGrayImage, m_xform.transform(), m_workingBoundingRect,
                                       OutsidePixels::assumeWeakColor(m_outsideBackgroundColor));
  } else {
    warpedGrayOutput = normalizeIlluminationGray(m_inputGrayImage, m_preCropAreaInOriginalCs, m_xform.transform(),
                                                 m_workingBoundingRect);
  }

  // Original image, but:
  // 1. In a format we can handle, that is grayscale, RGB32, ARGB32
  // 2. With illumination normalized over the content area, if required.
  // 3. With margins filled with white, if required.
  QImage normalizedOriginal;
  const QTransform workingToOrig
      = QTransform().translate(m_workingBoundingRect.left(), m_workingBoundingRect.top()) * m_xform.transformBack();
  if (!m_renderParams.normalizeIllumination()) {
    normalizedOriginal = (m_colorOriginal) ? m_inputOrigImage : m_inputGrayImage;
  } else {
    GrayImage normalizedGray = transformToGray(warpedGrayOutput, workingToOrig, m_inputOrigImage.rect(),
                                               OutsidePixels::assumeWeakColor(m_outsideBackgroundColor));
    if (!m_colorOriginal) {
      normalizedOriginal = normalizedGray;
    } else {
      normalizedOriginal = m_inputOrigImage;
      adjustBrightnessGrayscale(normalizedOriginal, normalizedGray);
      if (m_dbg) {
        m_dbg->add(normalizedOriginal, "norm_illum_color");
      }
    }

    m_outsideBackgroundColor
        = BackgroundColorCalculator::calcDominantBackgroundColor(normalizedOriginal, m_outCropAreaInOriginalCs);
  }

  m_status.throwIfCancelled();

  // Picture mask (white indicate a picture) in the same coordinates as
  // warpedGrayOutput.  Only built for Mixed mode.
  BinaryImage warpedBwMask;
  if (m_renderParams.mixedOutput()) {
    warpedBwMask = BinaryImage(m_workingBoundingRect.size(), BLACK);
    processPictureZones(warpedBwMask, pictureZones, warpedGrayOutput);
    if (m_dbg) {
      m_dbg->add(warpedBwMask, "warpedBwMask");
    }
    m_status.throwIfCancelled();

    if (autoPictureMask) {
      if (autoPictureMask->size() != m_targetSize) {
        BinaryImage(m_targetSize).swap(*autoPictureMask);
      }
      autoPictureMask->fill(BLACK);
      rasterOp<RopSrc>(*autoPictureMask, m_croppedContentRect, warpedBwMask, m_contentRectInWorkingCs.topLeft());
    }
    m_status.throwIfCancelled();

    modifyBinarizationMask(warpedBwMask, m_workingBoundingRect, pictureZones);
    if (m_dbg) {
      m_dbg->add(warpedBwMask, "warpedBwMask with zones");
    }
    m_status.throwIfCancelled();
  }

  if (m_dewarpingOptions.dewarpingMode() == AUTO) {
    distortionModel = buildAutoDistortionModel(warpedGrayOutput, workingToOrig);
  } else if (m_dewarpingOptions.dewarpingMode() == MARGINAL) {
    distortionModel = buildMarginalDistortionModel();
  }
  warpedGrayOutput = GrayImage();  // Save memory.

  QTransform rotateXform;
  QImage dewarped;
  try {
    dewarped = dewarp(QTransform(), normalizedOriginal, m_xform.transform(), distortionModel, depthPerception,
                      m_outsideBackgroundColor);

    const double deskewAngle = -findSkew(dewarped);
    rotateXform = Utils::rotate(deskewAngle, m_outRect);
    m_dewarpingOptions.setPostDeskewAngle(deskewAngle);
  } catch (const std::runtime_error&) {
    // Probably an impossible distortion model.  Let's fall back to a trivial one.
    setupTrivialDistortionModel(distortionModel);
    m_dewarpingOptions.setPostDeskewAngle(.0);

    dewarped = dewarp(QTransform(), normalizedOriginal, m_xform.transform(), distortionModel, depthPerception,
                      m_outsideBackgroundColor);
  }
  normalizedOriginal = QImage();  // Save memory.
  m_settings->setDewarpingOptions(m_pageId, m_dewarpingOptions);
  applyAffineTransform(dewarped, rotateXform, m_outsideBackgroundColor);
  normalizedOriginal = QImage();  // Save memory.
  if (m_dbg) {
    m_dbg->add(dewarped, "dewarped");
  }
  m_status.throwIfCancelled();

  auto mapper = std::make_shared<DewarpingPointMapper>(distortionModel, depthPerception.value(), m_xform.transform(),
                                                       m_croppedContentRect, rotateXform);
  const boost::function<QPointF(const QPointF&)> origToOutput(
      boost::bind(&DewarpingPointMapper::mapToDewarpedSpace, mapper, _1));

  BinaryImage dewarpingContentAreaMask(m_inputGrayImage.size(), BLACK);
  {
    fillMarginsInPlace(dewarpingContentAreaMask, m_contentAreaInOriginalCs, WHITE);
    dewarpingContentAreaMask = BinaryImage(dewarp(QTransform(), dewarpingContentAreaMask.toQImage(),
                                                  m_xform.transform(), distortionModel, depthPerception, Qt::white));
  }
  applyAffineTransform(dewarpingContentAreaMask, rotateXform, WHITE);

  if (m_renderParams.binaryOutput()) {
    QImage dewarpedAndMaybeSmoothed;
    // We only do smoothing if we are going to do binarization later.
    if (!m_renderParams.needSavitzkyGolaySmoothing()) {
      dewarpedAndMaybeSmoothed = dewarped;
    } else {
      dewarpedAndMaybeSmoothed = smoothToGrayscale(dewarped, m_dpi);
      if (m_dbg) {
        m_dbg->add(dewarpedAndMaybeSmoothed, "smoothed");
      }
      m_status.throwIfCancelled();
    }

    // don't destroy as it's needed for color segmentation
    if (!m_renderParams.needColorSegmentation()) {
      dewarped = QImage();
    }

    BinaryImage dewarpedBwContent = binarize(dewarpedAndMaybeSmoothed, dewarpingContentAreaMask);
    dewarpedAndMaybeSmoothed = QImage();  // Save memory.
    if (m_dbg) {
      m_dbg->add(dewarpedBwContent, "dewarpedBwContent");
    }
    m_status.throwIfCancelled();

    if (m_renderParams.needMorphologicalSmoothing()) {
      morphologicalSmoothInPlace(dewarpedBwContent);
      m_status.throwIfCancelled();
    }

    // It's important to keep despeckling the very last operation
    // affecting the binary part of the output. That's because
    // we will be reconstructing the input to this despeckling
    // operation from the final output file.
    maybeDespeckleInPlace(dewarpedBwContent, m_outRect, m_outRect, m_despeckleLevel, specklesImage, m_dpi);

    if (!m_renderParams.needColorSegmentation()) {
      if (!m_blackOnWhite) {
        dewarpedBwContent.invert();
      }

      applyFillZonesInPlace(dewarpedBwContent, fillZones, origToOutput);

      imageBuilder.setImage(dewarpedBwContent.toQImage());
    } else {
      QImage segmentedImage = segmentImage(dewarpedBwContent, dewarped);
      dewarped = QImage();
      dewarpedBwContent.release();

      if (m_renderParams.posterize()) {
        segmentedImage = posterizeImage(segmentedImage, m_outsideBackgroundColor);
      }

      if (!m_blackOnWhite) {
        segmentedImage.invertPixels();
      }

      applyFillZonesInPlace(segmentedImage, fillZones, origToOutput, false);
      if (m_dbg) {
        m_dbg->add(segmentedImage, "segmented_with_fill_zones");
      }
      m_status.throwIfCancelled();

      imageBuilder.setImage(segmentedImage);
    }
    return imageBuilder.build();
  }

  BinaryImage dewarpedBwMask;
  BinaryImage dewarpedBwContent;
  if (m_renderParams.mixedOutput()) {
    const QTransform origToWorkingCs
        = m_xform.transform() * QTransform().translate(-m_workingBoundingRect.left(), -m_workingBoundingRect.top());
    QTransform workingToOutputCs = QTransform().translate(m_workingBoundingRect.left(), m_workingBoundingRect.top());
    dewarpedBwMask = BinaryImage(dewarp(origToWorkingCs, warpedBwMask.toQImage(), workingToOutputCs, distortionModel,
                                        depthPerception, Qt::black));
    warpedBwMask.release();
    applyAffineTransform(dewarpedBwMask, rotateXform, BLACK);
    fillMarginsInPlace(dewarpedBwMask, dewarpingContentAreaMask, BLACK);
    if (m_dbg) {
      m_dbg->add(dewarpedBwMask, "dewarpedBwMask");
    }
    m_status.throwIfCancelled();

    if (m_renderParams.needBinarization()) {
      QImage dewarpedAndMaybeSmoothed;
      if (!m_renderParams.needSavitzkyGolaySmoothing()) {
        dewarpedAndMaybeSmoothed = dewarped;
      } else {
        dewarpedAndMaybeSmoothed = smoothToGrayscale(dewarped, m_dpi);
        if (m_dbg) {
          m_dbg->add(dewarpedAndMaybeSmoothed, "smoothed");
        }
        m_status.throwIfCancelled();
      }

      BinaryImage dewarpedBwMaskFilled(dewarpedBwMask);
      fillMarginsInPlace(dewarpedBwMaskFilled, dewarpingContentAreaMask, WHITE);
      dewarpedBwContent = binarize(dewarpedAndMaybeSmoothed, dewarpedBwMaskFilled);
      dewarpedBwMaskFilled.release();
      dewarpedAndMaybeSmoothed = QImage();  // Save memory.
      if (m_dbg) {
        m_dbg->add(dewarpedBwContent, "dewarpedBwContent");
      }
      m_status.throwIfCancelled();

      if (m_renderParams.needMorphologicalSmoothing()) {
        morphologicalSmoothInPlace(dewarpedBwContent);
      }

      // It's important to keep despeckling the very last operation
      // affecting the binary part of the output. That's because
      // we will be reconstructing the input to this despeckling
      // operation from the final output file.
      maybeDespeckleInPlace(dewarpedBwContent, m_outRect, m_croppedContentRect, m_despeckleLevel, specklesImage, m_dpi);

      if (!m_renderParams.normalizeIlluminationColor()) {
        m_outsideBackgroundColor = BackgroundColorCalculator::calcDominantBackgroundColor(
            m_colorOriginal ? m_inputOrigImage : m_inputGrayImage, m_outCropAreaInOriginalCs);

        {
          QImage origWithoutIllumination;
          if (m_colorOriginal) {
            origWithoutIllumination = m_inputOrigImage;
          } else {
            origWithoutIllumination = m_inputGrayImage;
          }
          dewarped = dewarp(QTransform(), origWithoutIllumination, m_xform.transform(), distortionModel,
                            depthPerception, m_outsideBackgroundColor);
        }
        applyAffineTransform(dewarped, rotateXform, m_outsideBackgroundColor);
        m_status.throwIfCancelled();
      }

      if (!m_renderParams.needColorSegmentation()) {
        if (m_renderParams.originalBackground()) {
          combineImages(dewarped, dewarpedBwContent);
        } else {
          combineImages(dewarped, dewarpedBwContent, dewarpedBwMask);
        }
      } else {
        QImage segmentedImage = segmentImage(dewarpedBwContent, dewarped);
        if (m_renderParams.posterize()) {
          segmentedImage = posterizeImage(segmentedImage, m_outsideBackgroundColor);
        }

        if (m_renderParams.originalBackground()) {
          combineImages(dewarped, segmentedImage);
        } else {
          combineImages(dewarped, segmentedImage, dewarpedBwMask);
        }
      }

      if (m_dbg) {
        m_dbg->add(dewarped, "combined");
      }

      if (m_renderParams.originalBackground()) {
        reserveBlackAndWhite(dewarped, dewarpedBwContent.inverted());
      } else {
        reserveBlackAndWhite(dewarped, dewarpedBwMask.inverted());
      }
      m_status.throwIfCancelled();
    }
  }

  if (m_renderParams.needBinarization() && !m_renderParams.originalBackground()) {
    m_outsideBackgroundColor = Qt::white;
  } else if (m_colorParams.colorCommonOptions().getFillingColor() == FILL_WHITE) {
    m_outsideBackgroundColor = m_blackOnWhite ? Qt::white : Qt::black;
  }
  fillMarginsInPlace(dewarped, dewarpingContentAreaMask, m_outsideBackgroundColor);

  if (!m_blackOnWhite) {
    dewarped.invertPixels();
  }

  if (m_renderParams.mixedOutput() && m_renderParams.needBinarization()) {
    applyFillZonesToMixedInPlace(dewarped, fillZones, origToOutput, dewarpedBwMask,
                                 !m_renderParams.needColorSegmentation());
  } else {
    applyFillZonesInPlace(dewarped, fillZones, origToOutput);
  }
  if (m_renderParams.originalBackground()) {
    applyFillZonesToMask(dewarpedBwContent, fillZones, origToOutput);
    rasterOp<RopAnd<RopSrc, RopDst>>(dewarpedBwContent, dewarpedBwMask);
  }
  if (m_dbg) {
    m_dbg->add(dewarped, "fillZones");
  }
  m_status.throwIfCancelled();

  if (m_renderParams.posterize() && !m_renderParams.needBinarization()) {
    dewarped = posterizeImage(dewarped);
  }

  if (m_renderParams.splitOutput()) {
    imageBuilder.setForegroundType(getForegroundType());
    if (!m_renderParams.originalBackground()) {
      imageBuilder.setForegroundMask(dewarpedBwMask);
    } else {
      imageBuilder.setForegroundMask(dewarpedBwContent);
      imageBuilder.setBackgroundMask(dewarpedBwMask.inverted());
    }
  }
  return imageBuilder.setImage(dewarped).build();
}

GrayImage OutputGenerator::Processor::normalizeIlluminationGray(const QImage& input,
                                                                const QPolygonF& areaToConsider,
                                                                const QTransform& xform,
                                                                const QRect& targetRect,
                                                                GrayImage* background) const {
  GrayImage toBeNormalized = transformToGray(input, xform, targetRect, OutsidePixels::assumeWeakNearest());
  if (m_dbg) {
    m_dbg->add(toBeNormalized, "toBeNormalized");
  }

  m_status.throwIfCancelled();

  QPolygonF transformedConsiderationArea = xform.map(areaToConsider);
  transformedConsiderationArea.translate(-targetRect.topLeft());

  const PolynomialSurface bgPs = estimateBackground(toBeNormalized, transformedConsiderationArea, m_status, m_dbg);
  m_status.throwIfCancelled();

  GrayImage bgImg(bgPs.render(toBeNormalized.size()));
  if (m_dbg) {
    m_dbg->add(bgImg, "background");
  }
  if (background) {
    *background = bgImg;
  }
  m_status.throwIfCancelled();

  grayRasterOp<RaiseAboveBackground>(bgImg, toBeNormalized);
  if (m_dbg) {
    m_dbg->add(bgImg, "normalized_illumination");
  }
  m_status.throwIfCancelled();
  return bgImg;
}

BinaryImage OutputGenerator::Processor::estimateBinarizationMask(const GrayImage& graySource,
                                                                 const QRect& sourceRect,
                                                                 const QRect& sourceSubRect) const {
  assert(sourceRect.contains(sourceSubRect));

  // If we need to strip some of the margins from a grayscale
  // image, we may actually do it without copying anything.
  // We are going to construct a QImage from existing data.
  // That image won't own that data, but graySource is not
  // going anywhere, so it's fine.

  GrayImage trimmedImage;

  if (sourceRect == sourceSubRect) {
    trimmedImage = graySource;  // Shallow copy.
  } else {
    // Sub-rectangle in input image coordinates.
    QRect relativeSubrect(sourceSubRect);
    relativeSubrect.moveTopLeft(sourceSubRect.topLeft() - sourceRect.topLeft());

    const int stride = graySource.stride();
    const int offset = relativeSubrect.top() * stride + relativeSubrect.left();

    trimmedImage = GrayImage(QImage(graySource.data() + offset, relativeSubrect.width(), relativeSubrect.height(),
                                    stride, QImage::Format_Indexed8));
  }

  m_status.throwIfCancelled();

  const QSize downscaledSize(to300dpi(trimmedImage.size(), m_dpi));

  // A 300dpi version of trimmedImage.
  GrayImage downscaledInput(scaleToGray(trimmedImage, downscaledSize));
  trimmedImage = GrayImage();  // Save memory.
  m_status.throwIfCancelled();

  // Light areas indicate pictures.
  GrayImage pictureAreas(detectPictures(downscaledInput));
  downscaledInput = GrayImage();  // Save memory.
  m_status.throwIfCancelled();

  const BinaryThreshold threshold(48);
  // Scale back to original size.
  pictureAreas = scaleToGray(pictureAreas, sourceSubRect.size());
  return BinaryImage(pictureAreas, threshold);
}

void OutputGenerator::Processor::modifyBinarizationMask(BinaryImage& bwMask,
                                                        const QRect& maskRect,
                                                        const ZoneSet& zones) const {
  QTransform xform = m_xform.transform();
  xform *= QTransform().translate(-maskRect.x(), -maskRect.y());

  using PLP = PictureLayerProperty;

  // Pass 1: ERASER1
  for (const Zone& zone : zones) {
    if (zone.properties().locateOrDefault<PLP>()->layer() == PLP::ERASER1) {
      const QPolygonF poly(zone.spline().toPolygon());
      PolygonRasterizer::fill(bwMask, BLACK, xform.map(poly), Qt::WindingFill);
    }
  }

  // Pass 2: PAINTER2
  for (const Zone& zone : zones) {
    if (zone.properties().locateOrDefault<PLP>()->layer() == PLP::PAINTER2) {
      const QPolygonF poly(zone.spline().toPolygon());
      PolygonRasterizer::fill(bwMask, WHITE, xform.map(poly), Qt::WindingFill);
    }
  }

  // Pass 1: ERASER3
  for (const Zone& zone : zones) {
    if (zone.properties().locateOrDefault<PLP>()->layer() == PLP::ERASER3) {
      const QPolygonF poly(zone.spline().toPolygon());
      PolygonRasterizer::fill(bwMask, BLACK, xform.map(poly), Qt::WindingFill);
    }
  }
}


/**
 * Set up a distortion model corresponding to the content rect,
 * which will result in no distortion correction.
 */
void OutputGenerator::Processor::setupTrivialDistortionModel(DistortionModel& distortionModel) const {
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

  std::vector<QPointF> topPolyline, bottomPolyline;
  topPolyline.push_back(poly[0]);     // top-left
  topPolyline.push_back(poly[1]);     // top-right
  bottomPolyline.push_back(poly[3]);  // bottom-left
  bottomPolyline.push_back(poly[2]);  // bottom-right
  distortionModel.setTopCurve(Curve(topPolyline));
  distortionModel.setBottomCurve(Curve(bottomPolyline));
}

CylindricalSurfaceDewarper OutputGenerator::Processor::createDewarper(const DistortionModel& distortionModel,
                                                                      const QTransform& distortionModelToTarget,
                                                                      double depthPerception) {
  if (distortionModelToTarget.isIdentity()) {
    return CylindricalSurfaceDewarper(distortionModel.topCurve().polyline(), distortionModel.bottomCurve().polyline(),
                                      depthPerception);
  }

  std::vector<QPointF> topPolyline(distortionModel.topCurve().polyline());
  std::vector<QPointF> bottomPolyline(distortionModel.bottomCurve().polyline());
  for (QPointF& pt : topPolyline) {
    pt = distortionModelToTarget.map(pt);
  }
  for (QPointF& pt : bottomPolyline) {
    pt = distortionModelToTarget.map(pt);
  }
  return CylindricalSurfaceDewarper(topPolyline, bottomPolyline, depthPerception);
}

/**
 * \param origToSrc Transformation from the original image coordinates
 *                    to the coordinate system of \p src image.
 * \param srcToOutput Transformation from the \p src image coordinates
 *                      to output image coordinates.
 * \param distortionModel Distortion model.
 * \param depthPerception Depth perception.
 * \param bgColor The color to use for areas outsize of \p src.
 * \param modified_content_rect A vertically shrunk version of outputContentRect().
 *                              See function definition for more details.
 */
QImage OutputGenerator::Processor::dewarp(const QTransform& origToSrc,
                                          const QImage& src,
                                          const QTransform& srcToOutput,
                                          const DistortionModel& distortionModel,
                                          const DepthPerception& depthPerception,
                                          const QColor& bgColor) const {
  const CylindricalSurfaceDewarper dewarper(createDewarper(distortionModel, origToSrc, depthPerception.value()));

  // Model domain is a rectangle in output image coordinates that
  // will be mapped to our curved quadrilateral.
  const QRect modelDomain(distortionModel.modelDomain(dewarper, origToSrc * srcToOutput, m_outRect).toRect());
  if (modelDomain.isEmpty()) {
    GrayImage out(src.size());
    out.fill(0xff);  // white
    return out;
  }
  return RasterDewarper::dewarp(src, m_outRect.size(), dewarper, modelDomain, bgColor);
}

GrayImage OutputGenerator::Processor::detectPictures(const GrayImage& input300dpi) const {
  // We stretch the range of gray levels to cover the whole
  // range of [0, 255].  We do it because we want text
  // and background to be equally far from the center
  // of the whole range.  Otherwise text printed with a big
  // font will be considered a picture.
  GrayImage stretched(stretchGrayRange(input300dpi, 0.01, 0.01));
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
  GrayImage grayGradient(dilated);
  dilated = GrayImage();
  eroded = GrayImage();
  if (m_dbg) {
    m_dbg->add(grayGradient, "grayGradient");
  }

  m_status.throwIfCancelled();

  GrayImage marker(erodeGray(grayGradient, QSize(35, 35), 0x00));
  if (m_dbg) {
    m_dbg->add(marker, "marker");
  }

  m_status.throwIfCancelled();

  seedFillGrayInPlace(marker, grayGradient, CONN8);
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

  GrayImage holesFilled(createFramedImage(reconstructed.size()));
  seedFillGrayInPlace(holesFilled, reconstructed, CONN8);
  reconstructed = GrayImage();
  if (m_dbg) {
    m_dbg->add(holesFilled, "holesFilled");
  }

  if (m_pictureShapeOptions.isHigherSearchSensitivity()) {
    GrayImage stretched2(stretchGrayRange(holesFilled, 5.0, 0.01));
    if (m_dbg) {
      m_dbg->add(stretched2, "stretched2");
    }
    return stretched2;
  }
  return holesFilled;
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
                                                                      const QPolygonF& cropArea,
                                                                      const BinaryImage* mask) const {
  QPainterPath path;
  path.addPolygon(cropArea);

  if (path.contains(image.rect())) {
    return adjustThreshold(BinaryThreshold::otsuThreshold(image));
  } else {
    BinaryImage modifiedMask(image.size(), BLACK);
    PolygonRasterizer::fillExcept(modifiedMask, WHITE, cropArea, Qt::WindingFill);
    modifiedMask = erodeBrick(modifiedMask, QSize(3, 3), WHITE);

    if (mask) {
      rasterOp<RopAnd<RopSrc, RopDst>>(modifiedMask, *mask);
    }
    return calcBinarizationThreshold(image, modifiedMask);
  }
}

void OutputGenerator::Processor::morphologicalSmoothInPlace(BinaryImage& binImg) const {
  // When removing black noise, remove small ones first.

  {
    const char pattern[]
        = "XXX"
          " - "
          "   ";
    hitMissReplaceAllDirections(binImg, pattern, 3, 3);
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
    hitMissReplaceAllDirections(binImg, pattern, 3, 6);
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
    hitMissReplaceAllDirections(binImg, pattern, 3, 9);
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
    hitMissReplaceAllDirections(binImg, pattern, 3, 9);
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
    hitMissReplaceAllDirections(binImg, pattern, 3, 6);
  }

  m_status.throwIfCancelled();

  {
    const char pattern[]
        = "   "
          "X+X"
          "XXX";
    hitMissReplaceAllDirections(binImg, pattern, 3, 3);
  }

  if (m_dbg) {
    m_dbg->add(binImg, "edges_smoothed");
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
      const BinaryThreshold bwThresh(BinaryThreshold::otsuThreshold(hist));

      binarized = BinaryImage(image, adjustThreshold(bwThresh));
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
    case EDGEPLUS: {
      QSize windowsSize = QSize(blackWhiteOptions.getWindowSize(), blackWhiteOptions.getWindowSize());
      double sauvolaCoef = blackWhiteOptions.getSauvolaCoef();

      binarized = binarizeEdgePlus(image, windowsSize, sauvolaCoef);
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
                                                 const QPolygonF& cropArea,
                                                 const BinaryImage* mask) const {
  QPainterPath path;
  path.addPolygon(cropArea);

  if (path.contains(image.rect()) && !mask) {
    return binarize(image);
  } else {
    BinaryImage modifiedMask(image.size(), BLACK);
    PolygonRasterizer::fillExcept(modifiedMask, WHITE, cropArea, Qt::WindingFill);
    modifiedMask = erodeBrick(modifiedMask, QSize(3, 3), WHITE);

    if (mask) {
      rasterOp<RopAnd<RopSrc, RopDst>>(modifiedMask, *mask);
    }
    return binarize(image, modifiedMask);
  }
}

/**
 * \brief Remove small connected components that are considered to be garbage.
 *
 * Both the size and the distance to other components are taken into account.
 *
 * \param[in,out] image The image to despeckle.
 * \param imageRect The rectangle corresponding to \p image in the same
 *        coordinate system where m_contentRect and m_cropRect are defined.
 * \param maskRect The area within the image to consider.  Defined not
 *        relative to \p image, but in the same coordinate system where
 *        m_contentRect and m_cropRect are defined.  This only affects
 *        \p specklesImg, if provided.
 * \param level Despeckling aggressiveness.
 * \param specklesImg If provided, the removed black speckles will be written
 *        there.  The speckles image is always considered to correspond
 *        to m_cropRect, so it will have the size of m_cropRect.size().
 *        Only the area within \p maskRect will be copied to \p specklesImg.
 *        The rest will be filled with white.
 * \param dpi The DPI of the input image.  See the note below.
 * \param status Task status.
 * \param dbg An optional sink for debugging images.
 *
 * \note This function only works effectively when the DPI is symmetric,
 * that is, its horizontal and vertical components are equal.
 */
void OutputGenerator::Processor::maybeDespeckleInPlace(BinaryImage& image,
                                                       const QRect& imageRect,
                                                       const QRect& maskRect,
                                                       double level,
                                                       BinaryImage* specklesImg,
                                                       const Dpi& dpi) const {
  const QRect srcRect(maskRect.translated(-imageRect.topLeft()));
  const QRect dstRect(maskRect);

  if (specklesImg) {
    BinaryImage(m_outRect.size(), WHITE).swap(*specklesImg);
    if (!maskRect.isEmpty()) {
      rasterOp<RopSrc>(*specklesImg, dstRect, image, srcRect.topLeft());
    }
  }

  if (level != 0) {
    Despeckle::despeckleInPlace(image, dpi, level, m_status, m_dbg);

    if (m_dbg) {
      m_dbg->add(image, "despeckled");
    }
  }

  if (specklesImg) {
    if (!maskRect.isEmpty()) {
      rasterOp<RopSubtract<RopDst, RopSrc>>(*specklesImg, dstRect, image, srcRect.topLeft());
    }
  }

  m_status.throwIfCancelled();
}

double OutputGenerator::Processor::findSkew(const QImage& image) const {
  if (m_dewarpingOptions.needPostDeskew()
      && ((m_dewarpingOptions.dewarpingMode() == MARGINAL) || (m_dewarpingOptions.dewarpingMode() == MANUAL))) {
    const BinaryImage bwImage(image, BinaryThreshold::otsuThreshold(GrayscaleHistogram(image)));
    const Skew skew = SkewFinder().findSkew(bwImage);
    if ((skew.angle() != .0) && (skew.confidence() >= Skew::GOOD_CONFIDENCE)) {
      return skew.angle();
    }
  }
  return .0;
}

QImage OutputGenerator::Processor::segmentImage(const BinaryImage& image, const QImage& colorImage) const {
  const BlackWhiteOptions::ColorSegmenterOptions& segmenterOptions
      = m_colorParams.blackWhiteOptions().getColorSegmenterOptions();
  ColorSegmenter segmenter(m_dpi, segmenterOptions.getNoiseReduction(), segmenterOptions.getRedThresholdAdjustment(),
                           segmenterOptions.getGreenThresholdAdjustment(),
                           segmenterOptions.getBlueThresholdAdjustment());

  QImage segmented;
  {
    if (m_colorOriginal) {
      segmented = segmenter.segment(image, colorImage);
    } else {
      segmented = segmenter.segment(image, GrayImage(colorImage));
    }
  }

  if (m_dbg) {
    m_dbg->add(segmented, "segmented");
  }
  m_status.throwIfCancelled();
  return segmented;
}

QImage OutputGenerator::Processor::posterizeImage(const QImage& image, const QColor& backgroundColor) const {
  const ColorCommonOptions::PosterizationOptions& posterizationOptions
      = m_colorParams.colorCommonOptions().getPosterizationOptions();
  Posterizer posterizer(posterizationOptions.getLevel(), posterizationOptions.isNormalizationEnabled(),
                        posterizationOptions.isForceBlackAndWhite(), 0, qRound(backgroundColor.lightnessF() * 255));

  QImage posterized = posterizer.posterize(image);

  if (m_dbg) {
    m_dbg->add(posterized, "posterized");
  }
  m_status.throwIfCancelled();
  return posterized;
}

void OutputGenerator::Processor::processPictureZones(BinaryImage& mask, ZoneSet& pictureZones, const GrayImage& image) {
  if ((m_pictureShapeOptions.getPictureShape() != RECTANGULAR_SHAPE) || !m_outputProcessingParams.isAutoZonesFound()) {
    if (m_pictureShapeOptions.getPictureShape() != OFF_SHAPE) {
      mask = estimateBinarizationMask(image, m_workingBoundingRect, m_workingBoundingRect);
    }

    removeAutoPictureZones(pictureZones);
    m_settings->setPictureZones(m_pageId, pictureZones);
    m_outputProcessingParams.setAutoZonesFound(false);
    m_settings->setOutputProcessingParams(m_pageId, m_outputProcessingParams);
  }
  if ((m_pictureShapeOptions.getPictureShape() == RECTANGULAR_SHAPE) && !m_outputProcessingParams.isAutoZonesFound()) {
    std::vector<QRect> areas = findRectAreas(mask, WHITE, m_pictureShapeOptions.getSensitivity());

    const QTransform fromWorkingCs = [this]() {
      QTransform toWorkingCs = m_xform.transform();
      toWorkingCs *= QTransform().translate(-m_workingBoundingRect.x(), -m_workingBoundingRect.y());
      return toWorkingCs.inverted();
    }();

    for (const QRect& area : areas) {
      pictureZones.add(createPictureZoneFromPoly(fromWorkingCs.map(QRectF(area))));
    }
    m_settings->setPictureZones(m_pageId, pictureZones);
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

DistortionModel OutputGenerator::Processor::buildAutoDistortionModel(const GrayImage& warpedGrayOutput,
                                                                     const QTransform& toOriginal) const {
  DistortionModelBuilder modelBuilder(Vec2d(0, 1));

  TextLineTracer::trace(warpedGrayOutput, m_dpi, m_contentRectInWorkingCs, modelBuilder, m_status, m_dbg);
  modelBuilder.transform(toOriginal);

  TopBottomEdgeTracer::trace(m_inputGrayImage, modelBuilder.verticalBounds(), modelBuilder, m_status, m_dbg);

  DistortionModel distortionModel = modelBuilder.tryBuildModel(m_dbg, &m_inputGrayImage.toQImage());
  if (!distortionModel.isValid()) {
    setupTrivialDistortionModel(distortionModel);
  }

  BinaryImage bwImage(m_inputGrayImage, BinaryThreshold(64));

  QTransform transform = m_xform.preRotation().transform(bwImage.size());
  QTransform invTransform = transform.inverted();

  int degrees = m_xform.preRotation().toDegrees();
  bwImage = orthogonalRotation(bwImage, degrees);

  const std::vector<QPointF>& topPolyline0 = distortionModel.topCurve().polyline();
  const std::vector<QPointF>& bottomPolyline0 = distortionModel.bottomCurve().polyline();

  std::vector<QPointF> topPolyline;
  std::vector<QPointF> bottomPolyline;

  for (auto i : topPolyline0) {
    topPolyline.push_back(transform.map(i));
  }

  for (auto i : bottomPolyline0) {
    bottomPolyline.push_back(transform.map(i));
  }

  QString stAngle;

  float maxAngle = 2.75;

  if ((m_pageId.subPage() == PageId::SINGLE_PAGE) || (m_pageId.subPage() == PageId::LEFT_PAGE)) {
    float vertSkewAngleLeft = vertBorderSkewAngle(topPolyline.front(), bottomPolyline.front());

    stAngle.setNum(vertSkewAngleLeft);


    if (vertSkewAngleLeft > maxAngle) {
      auto topX = static_cast<float>(topPolyline.front().x());
      auto bottomX = static_cast<float>(bottomPolyline.front().x());

      if (topX < bottomX) {
        std::vector<QPointF> newBottomPolyline;

        QPointF pt(topX, bottomPolyline.front().y());

        newBottomPolyline.push_back(pt);

        for (auto i : bottomPolyline) {
          newBottomPolyline.push_back(invTransform.map(i));
        }

        distortionModel.setBottomCurve(Curve(newBottomPolyline));
      } else {
        std::vector<QPointF> newTopPolyline;

        QPointF pt(bottomX, topPolyline.front().y());

        newTopPolyline.push_back(pt);

        for (auto i : topPolyline) {
          newTopPolyline.push_back(invTransform.map(i));
        }

        distortionModel.setBottomCurve(Curve(newTopPolyline));
      }
    }
  } else {
    float vertSkewAngleRight = vertBorderSkewAngle(topPolyline.back(), bottomPolyline.back());

    stAngle.setNum(vertSkewAngleRight);


    if (vertSkewAngleRight > maxAngle) {
      auto topX = static_cast<float>(topPolyline.back().x());
      auto bottomX = static_cast<float>(bottomPolyline.back().x());

      if (topX > bottomX) {
        std::vector<QPointF> newBottomPolyline;

        QPointF pt(topX, bottomPolyline.back().y());

        for (auto i : bottomPolyline) {
          newBottomPolyline.push_back(invTransform.map(i));
        }

        newBottomPolyline.push_back(pt);

        distortionModel.setBottomCurve(Curve(newBottomPolyline));
      } else {
        std::vector<QPointF> newTopPolyline;

        QPointF pt(bottomX, topPolyline.back().y());

        for (auto i : topPolyline) {
          newTopPolyline.push_back(invTransform.map(i));
        }

        newTopPolyline.push_back(pt);

        distortionModel.setBottomCurve(Curve(newTopPolyline));
      }
    }
  }
  return distortionModel;
}

DistortionModel OutputGenerator::Processor::buildMarginalDistortionModel() const {
  BinaryImage bwImage(m_inputGrayImage, BinaryThreshold(64));

  QTransform transform = m_xform.preRotation().transform(bwImage.size());
  QTransform invTransform = transform.inverted();

  int degrees = m_xform.preRotation().toDegrees();
  bwImage = orthogonalRotation(bwImage, degrees);

  DistortionModel distortionModel;
  setupTrivialDistortionModel(distortionModel);

  int maxRedPoints = 5;
  XSpline topSpline;

  const std::vector<QPointF>& topPolyline = distortionModel.topCurve().polyline();

  const QLineF topLine(transform.map(topPolyline.front()), transform.map(topPolyline.back()));

  topSpline.appendControlPoint(topLine.p1(), 0);

  if ((m_pageId.subPage() == PageId::SINGLE_PAGE) || (m_pageId.subPage() == PageId::LEFT_PAGE)) {
    for (int i = 29 - maxRedPoints; i < 29; i++) {
      topSpline.appendControlPoint(topLine.pointAt((float) i / 29.0), 1);
    }
  } else {
    for (int i = 1; i <= maxRedPoints; i++) {
      topSpline.appendControlPoint(topLine.pointAt((float) i / 29.0), 1);
    }
  }

  topSpline.appendControlPoint(topLine.p2(), 0);

  for (int i = 0; i <= topSpline.numSegments(); i++) {
    movePointToTopMargin(bwImage, topSpline, i);
  }

  for (int i = 0; i <= topSpline.numSegments(); i++) {
    topSpline.moveControlPoint(i, invTransform.map(topSpline.controlPointPosition(i)));
  }

  distortionModel.setTopCurve(Curve(topSpline));


  XSpline bottomSpline;

  const std::vector<QPointF>& bottomPolyline = distortionModel.bottomCurve().polyline();

  const QLineF bottomLine(transform.map(bottomPolyline.front()), transform.map(bottomPolyline.back()));

  bottomSpline.appendControlPoint(bottomLine.p1(), 0);

  if ((m_pageId.subPage() == PageId::SINGLE_PAGE) || (m_pageId.subPage() == PageId::LEFT_PAGE)) {
    for (int i = 29 - maxRedPoints; i < 29; i++) {
      bottomSpline.appendControlPoint(topLine.pointAt((float) i / 29.0), 1);
    }
  } else {
    for (int i = 1; i <= maxRedPoints; i++) {
      bottomSpline.appendControlPoint(topLine.pointAt((float) i / 29.0), 1);
    }
  }

  bottomSpline.appendControlPoint(bottomLine.p2(), 0);

  for (int i = 0; i <= bottomSpline.numSegments(); i++) {
    movePointToBottomMargin(bwImage, bottomSpline, i);
  }

  for (int i = 0; i <= bottomSpline.numSegments(); i++) {
    bottomSpline.moveControlPoint(i, invTransform.map(bottomSpline.controlPointPosition(i)));
  }

  distortionModel.setBottomCurve(Curve(bottomSpline));

  if (!distortionModel.isValid()) {
    setupTrivialDistortionModel(distortionModel);
  }

  if (m_dbg) {
    QImage outImage(bwImage.toQImage().convertToFormat(QImage::Format_RGB32));
    for (int i = 0; i <= topSpline.numSegments(); i++) {
      drawPoint(outImage, topSpline.controlPointPosition(i));
    }
    for (int i = 0; i <= bottomSpline.numSegments(); i++) {
      drawPoint(outImage, bottomSpline.controlPointPosition(i));
    }
    m_dbg->add(outImage, "marginal dewarping");
  }
  return distortionModel;
}
}  // namespace output
