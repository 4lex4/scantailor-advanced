
#include "ColorSegmenter.h"
#include <QImage>
#include <cmath>
#include "BinaryImage.h"
#include "Dpi.h"
#include "GrayImage.h"
#include "InfluenceMap.h"
#include "RasterOp.h"

namespace imageproc {
struct ColorSegmenter::Component {
  uint32_t pixelsCount;

  Component() : pixelsCount(0) {}

  inline int square() const { return pixelsCount; }
};

struct ColorSegmenter::BoundingBox {
  int top;
  int left;
  int bottom;
  int right;

  BoundingBox() {
    top = left = std::numeric_limits<int>::max();
    bottom = right = std::numeric_limits<int>::min();
  }

  inline int width() const { return right - left + 1; }

  inline int height() const { return bottom - top + 1; }

  inline void extend(int x, int y) {
    top = std::min(top, y);
    left = std::min(left, x);
    bottom = std::max(bottom, y);
    right = std::max(right, x);
  }
};

struct RgbColor {
  uint32_t red;
  uint32_t green;
  uint32_t blue;

  RgbColor() : red(0), green(0), blue(0) {}
};


/*=============================== ColorSegmenter::Settings ==================================*/

ColorSegmenter::Settings::Settings(const Dpi& dpi, const int noiseThreshold) {
  const int average_dpi = (dpi.horizontal() + dpi.vertical()) / 2;
  const double dpi_factor = average_dpi / 300.0;

  m_minAverageWidthThreshold = 1.5 * dpi_factor;
  m_bigObjectThreshold = qRound(std::pow(noiseThreshold, std::sqrt(2)) * dpi_factor);
}

inline bool ColorSegmenter::Settings::eligibleForDelete(const ColorSegmenter::Component& component,
                                                        const ColorSegmenter::BoundingBox& boundingBox) const {
  if (component.pixelsCount <= m_bigObjectThreshold) {
    return true;
  }

  double squareRelation = double(component.square()) / (boundingBox.height() * boundingBox.width());
  double averageWidth = std::min(boundingBox.height(), boundingBox.width()) * squareRelation;

  return (averageWidth <= m_minAverageWidthThreshold);
}

/*=============================== ColorSegmenter ==================================*/

ColorSegmenter::ColorSegmenter(const BinaryImage& image,
                               const QImage& originalImage,
                               const Dpi& dpi,
                               const int noiseThreshold,
                               const int redThresholdAdjustment,
                               const int greenThresholdAdjustment,
                               const int blueThresholdAdjustment)
    : m_settings(dpi, noiseThreshold) {
  if (image.size() != originalImage.size()) {
    throw std::invalid_argument("ColorSegmenter: images size doesn't match.");
  }
  if ((originalImage.format() != QImage::Format_Indexed8) && (originalImage.format() != QImage::Format_RGB32)
      && (originalImage.format() != QImage::Format_ARGB32)) {
    throw std::invalid_argument("Error: wrong image format.");
  }
  if (originalImage.format() == QImage::Format_Indexed8) {
    if (originalImage.isGrayscale()) {
      fromGrayscale(image, GrayImage(originalImage));
      return;
    } else {
      throw std::invalid_argument("Error: wrong image format.");
    }
  }
  fromRgb(image, originalImage, redThresholdAdjustment, greenThresholdAdjustment, blueThresholdAdjustment);
}

ColorSegmenter::ColorSegmenter(const BinaryImage& image,
                               const GrayImage& originalImage,
                               const Dpi& dpi,
                               const int noiseThreshold)
    : m_settings(dpi, noiseThreshold) {
  if (image.size() != originalImage.size()) {
    throw std::invalid_argument("ColorSegmenter: images size doesn't match.");
  }
  fromGrayscale(image, originalImage);
}

QImage ColorSegmenter::getImage() const {
  if (m_originalImage.format() == QImage::Format_Indexed8) {
    return buildGrayImage();
  } else {
    return buildRgbImage();
  }
}

GrayImage ColorSegmenter::getRgbChannel(const QImage& image, const ColorSegmenter::RgbChannel channel) {
  if ((image.format() != QImage::Format_RGB32) && (image.format() != QImage::Format_ARGB32)) {
    throw std::invalid_argument("ColorSegmenter: wrong image format.");
  }

  GrayImage dst(image.size());
  const auto* img_line = reinterpret_cast<const uint32_t*>(image.bits());
  const int img_stride = image.bytesPerLine() / sizeof(uint32_t);
  uint8_t* dst_line = dst.data();
  const int dst_stride = dst.stride();
  const int width = image.width();
  const int height = image.height();

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      switch (channel) {
        case RED_CHANNEL:
          dst_line[x] = static_cast<uint8_t>((img_line[x] >> 16) & 0xff);
          break;
        case GREEN_CHANNEL:
          dst_line[x] = static_cast<uint8_t>((img_line[x] >> 8) & 0xff);
          break;
        case BLUE_CHANNEL:
          dst_line[x] = static_cast<uint8_t>(img_line[x] & 0xff);
          break;
      }
    }
    img_line += img_stride;
    dst_line += dst_stride;
  }

  return dst;
}

void ColorSegmenter::fromRgb(const BinaryImage& image,
                             const QImage& originalImage,
                             const int redThresholdAdjustment,
                             const int greenThresholdAdjustment,
                             const int blueThresholdAdjustment) {
  this->m_originalImage = originalImage;

  BinaryImage redComponent;
  {
    GrayImage redChannel = getRgbChannel(originalImage, RED_CHANNEL);
    BinaryThreshold redThreshold = BinaryThreshold::otsuThreshold(redChannel);
    redComponent = BinaryImage(redChannel, adjustThreshold(redThreshold, redThresholdAdjustment));
    rasterOp<RopAnd<RopSrc, RopDst>>(redComponent, image);
  }
  BinaryImage greenComponent;
  {
    GrayImage greenChannel = getRgbChannel(originalImage, GREEN_CHANNEL);
    BinaryThreshold greenThreshold = BinaryThreshold::otsuThreshold(greenChannel);
    greenComponent = BinaryImage(greenChannel, adjustThreshold(greenThreshold, greenThresholdAdjustment));
    rasterOp<RopAnd<RopSrc, RopDst>>(greenComponent, image);
  }
  BinaryImage blueComponent;
  {
    GrayImage blueChannel = getRgbChannel(originalImage, BLUE_CHANNEL);
    BinaryThreshold blueThreshold = BinaryThreshold::otsuThreshold(blueChannel);
    blueComponent = BinaryImage(blueChannel, adjustThreshold(blueThreshold, blueThresholdAdjustment));
    rasterOp<RopAnd<RopSrc, RopDst>>(blueComponent, image);
  }
  BinaryImage yellowComponent(redComponent);
  rasterOp<RopAnd<RopSrc, RopDst>>(yellowComponent, greenComponent);
  BinaryImage magentaComponent(redComponent);
  rasterOp<RopAnd<RopSrc, RopDst>>(magentaComponent, blueComponent);
  BinaryImage cyanComponent(greenComponent);
  rasterOp<RopAnd<RopSrc, RopDst>>(cyanComponent, blueComponent);

  BinaryImage blackComponent(blueComponent);
  rasterOp<RopAnd<RopSrc, RopDst>>(blackComponent, yellowComponent);

  rasterOp<RopSubtract<RopDst, RopSrc>>(redComponent, blackComponent);
  rasterOp<RopSubtract<RopDst, RopSrc>>(greenComponent, blackComponent);
  rasterOp<RopSubtract<RopDst, RopSrc>>(blueComponent, blackComponent);
  rasterOp<RopSubtract<RopDst, RopSrc>>(yellowComponent, blackComponent);
  rasterOp<RopSubtract<RopDst, RopSrc>>(magentaComponent, blackComponent);
  rasterOp<RopSubtract<RopDst, RopSrc>>(cyanComponent, blackComponent);

  rasterOp<RopSubtract<RopDst, RopSrc>>(redComponent, yellowComponent);
  rasterOp<RopSubtract<RopDst, RopSrc>>(redComponent, magentaComponent);
  rasterOp<RopSubtract<RopDst, RopSrc>>(greenComponent, yellowComponent);
  rasterOp<RopSubtract<RopDst, RopSrc>>(greenComponent, cyanComponent);
  rasterOp<RopSubtract<RopDst, RopSrc>>(blueComponent, magentaComponent);
  rasterOp<RopSubtract<RopDst, RopSrc>>(blueComponent, cyanComponent);

  m_segmentsMap = ConnectivityMap(blackComponent, CONN8);
  m_segmentsMap.addComponents(yellowComponent, CONN8);
  m_segmentsMap.addComponents(magentaComponent, CONN8);
  m_segmentsMap.addComponents(cyanComponent, CONN8);
  m_segmentsMap.addComponents(redComponent, CONN8);
  m_segmentsMap.addComponents(greenComponent, CONN8);
  m_segmentsMap.addComponents(blueComponent, CONN8);

  reduceNoise();

  {
    // extend the map and fill unlabeled components.
    InfluenceMap influenceMap(m_segmentsMap, image);
    m_segmentsMap = influenceMap;
  }

  BinaryImage remainingComponents(image);
  rasterOp<RopSubtract<RopDst, RopSrc>>(remainingComponents, m_segmentsMap.getBinaryMask());
  m_segmentsMap.addComponents(remainingComponents, CONN8);
}

void ColorSegmenter::fromGrayscale(const BinaryImage& image, const GrayImage& originalImage) {
  this->m_originalImage = originalImage;
  this->m_segmentsMap = ConnectivityMap(image, CONN8);
}

void ColorSegmenter::reduceNoise() {
  std::vector<Component> components(m_segmentsMap.maxLabel() + 1);
  std::vector<BoundingBox> boundingBoxes(m_segmentsMap.maxLabel() + 1);

  const QSize size = m_segmentsMap.size();
  const int width = size.width();
  const int height = size.height();

  // Count the number of pixels and the bounding rect of each component.
  const uint32_t* map_line = m_segmentsMap.data();
  const int map_stride = m_segmentsMap.stride();
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      const uint32_t label = map_line[x];
      ++components[label].pixelsCount;
      boundingBoxes[label].extend(x, y);
    }
    map_line += map_stride;
  }

  // creating set of labels determining components to be removed
  std::unordered_set<uint32_t> labels;
  for (uint32_t label = 1; label <= m_segmentsMap.maxLabel(); ++label) {
    if (m_settings.eligibleForDelete(components[label], boundingBoxes[label])) {
      labels.insert(label);
    }
  }

  m_segmentsMap.removeComponents(labels);
}

QImage ColorSegmenter::buildRgbImage() const {
  if (m_originalImage.size().isEmpty()) {
    return QImage();
  }

  const int width = m_originalImage.width();
  const int height = m_originalImage.height();

  std::vector<uint32_t> colorMap(m_segmentsMap.maxLabel() + 1, 0);

  {
    const uint32_t* map_line = m_segmentsMap.data();
    const int map_stride = m_segmentsMap.stride();

    const auto* img_line = reinterpret_cast<const uint32_t*>(m_originalImage.bits());
    const int img_stride = m_originalImage.bytesPerLine() / sizeof(uint32_t);

    std::vector<Component> components(m_segmentsMap.maxLabel() + 1);
    std::vector<RgbColor> rgbSumMap(m_segmentsMap.maxLabel() + 1);
    for (int y = 0; y < height; ++y) {
      for (int x = 0; x < width; ++x) {
        const uint32_t label = map_line[x];
        if (label == 0) {
          continue;
        }

        ++components[label].pixelsCount;
        rgbSumMap[label].red += static_cast<uint8_t>((img_line[x] >> 16) & 0xff);
        rgbSumMap[label].green += static_cast<uint8_t>((img_line[x] >> 8) & 0xff);
        rgbSumMap[label].blue += static_cast<uint8_t>(img_line[x] & 0xff);
      }
      map_line += map_stride;
      img_line += img_stride;
    }

    for (int label = 1; label <= m_segmentsMap.maxLabel(); label++) {
      const auto red = static_cast<uint32_t>(std::round(double(rgbSumMap[label].red) / components[label].pixelsCount));
      const auto green
          = static_cast<uint32_t>(std::round(double(rgbSumMap[label].green) / components[label].pixelsCount));
      const auto blue
          = static_cast<uint32_t>(std::round(double(rgbSumMap[label].blue) / components[label].pixelsCount));

      colorMap[label] = (red << 16) | (green << 8) | (blue);
    }
  }

  QImage dst(m_originalImage.size(), QImage::Format_ARGB32_Premultiplied);
  dst.fill(Qt::white);

  auto* dst_line = reinterpret_cast<uint32_t*>(dst.bits());
  const int dst_stride = dst.bytesPerLine() / sizeof(uint32_t);

  const uint32_t* map_line = m_segmentsMap.data();
  const int map_stride = m_segmentsMap.stride();

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      const uint32_t label = map_line[x];
      if (label == 0) {
        continue;
      }

      dst_line[x] = colorMap[label];
    }
    map_line += map_stride;
    dst_line += dst_stride;
  }

  return dst.convertToFormat(QImage::Format_RGB32);
}

QImage ColorSegmenter::buildGrayImage() const {
  if (m_originalImage.size().isEmpty()) {
    return QImage();
  }

  const int width = m_originalImage.width();
  const int height = m_originalImage.height();

  std::vector<uint8_t> colorMap(m_segmentsMap.maxLabel() + 1, 0);

  {
    const uint32_t* map_line = m_segmentsMap.data();
    const int map_stride = m_segmentsMap.stride();

    const auto* img_line = m_originalImage.bits();
    const int img_stride = m_originalImage.bytesPerLine();

    std::vector<Component> components(m_segmentsMap.maxLabel() + 1);
    std::vector<uint32_t> graySumMap(m_segmentsMap.maxLabel() + 1, 0);
    for (int y = 0; y < height; ++y) {
      for (int x = 0; x < width; ++x) {
        const uint32_t label = map_line[x];
        if (label == 0) {
          continue;
        }

        ++components[label].pixelsCount;
        graySumMap[label] += img_line[x];
      }
      map_line += map_stride;
      img_line += img_stride;
    }

    for (int label = 1; label <= m_segmentsMap.maxLabel(); label++) {
      colorMap[label] = static_cast<uint8_t>(std::round(double(graySumMap[label]) / components[label].pixelsCount));
    }
  }

  GrayImage dst(m_originalImage.size());
  dst.fill(0xff);

  uint8_t* dst_line = dst.data();
  const int dst_stride = dst.stride();

  const uint32_t* map_line = m_segmentsMap.data();
  const int map_stride = m_segmentsMap.stride();

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      const uint32_t label = map_line[x];
      if (label == 0) {
        continue;
      }

      dst_line[x] = colorMap[label];
    }
    map_line += map_stride;
    dst_line += dst_stride;
  }

  return dst;
}

inline BinaryThreshold ColorSegmenter::adjustThreshold(const BinaryThreshold threshold, const int adjustment) {
  return qBound(1, int(threshold) + adjustment, 255);
}

}  // namespace imageproc