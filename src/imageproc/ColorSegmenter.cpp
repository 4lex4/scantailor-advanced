// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "ColorSegmenter.h"
#include <QImage>
#include <cmath>
#include "BinaryImage.h"
#include "BinaryThreshold.h"
#include "ConnectivityMap.h"
#include "GrayImage.h"
#include "ImageCombination.h"
#include "InfluenceMap.h"
#include "RasterOp.h"

namespace imageproc {
ColorSegmenter::ColorSegmenter(const Dpi& dpi,
                               const int noiseThreshold,
                               const int redThresholdAdjustment,
                               const int greenThresholdAdjustment,
                               const int blueThresholdAdjustment)
    : m_dpi(dpi),
      m_noiseThreshold(noiseThreshold),
      m_redThresholdAdjustment(redThresholdAdjustment),
      m_greenThresholdAdjustment(greenThresholdAdjustment),
      m_blueThresholdAdjustment(blueThresholdAdjustment) {}

ColorSegmenter::ColorSegmenter(const Dpi& dpi, const int noiseThreshold)
    : ColorSegmenter(dpi, noiseThreshold, 0, 0, 0) {}

namespace {
struct Component {
  uint32_t size;

  Component() : size(0) {}

  inline uint32_t square() const { return size; }
};

struct BoundingBox {
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

class ComponentCleaner {
 public:
  explicit ComponentCleaner(const Dpi& dpi, int noiseThreshold);

  inline bool eligibleForDelete(const Component& component, const BoundingBox& boundingBox) const;

 private:
  /**
   * Defines the minimum average width threshold.
   * When a component has lower that, it will be erased.
   */
  double m_minAverageWidthThreshold;

  /**
   * Defines the minimum square in pixels.
   * If a component has lower that, it will be erased.
   */
  uint32_t m_bigObjectThreshold;
};

ComponentCleaner::ComponentCleaner(const Dpi& dpi, const int noiseThreshold) {
  const int average_dpi = (dpi.horizontal() + dpi.vertical()) / 2;
  const double dpi_factor = average_dpi / 300.0;

  m_minAverageWidthThreshold = 1.5 * dpi_factor;
  m_bigObjectThreshold = qRound(std::pow(noiseThreshold, std::sqrt(2)) * dpi_factor);
}

bool ComponentCleaner::eligibleForDelete(const Component& component, const BoundingBox& boundingBox) const {
  if (component.size <= m_bigObjectThreshold) {
    return true;
  }

  double squareRelation = double(component.square()) / (boundingBox.height() * boundingBox.width());
  double averageWidth = std::min(boundingBox.height(), boundingBox.width()) * squareRelation;

  return (averageWidth <= m_minAverageWidthThreshold);
}

enum RgbChannel { RED_CHANNEL, GREEN_CHANNEL, BLUE_CHANNEL };

GrayImage extractRgbChannel(const QImage& image, const RgbChannel channel) {
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

inline BinaryThreshold adjustThreshold(const BinaryThreshold threshold, const int adjustment) {
  return qBound(1, int(threshold) + adjustment, 255);
}

void reduceNoise(ConnectivityMap& segmentsMap, const Dpi& dpi, const int noiseThreshold) {
  const ComponentCleaner componentCleaner(dpi, noiseThreshold);
  std::vector<Component> components(segmentsMap.maxLabel() + 1);
  std::vector<BoundingBox> boundingBoxes(segmentsMap.maxLabel() + 1);

  const QSize size = segmentsMap.size();
  const int width = size.width();
  const int height = size.height();

  // Count the number of pixels and the bounding rect of each component.
  const uint32_t* map_line = segmentsMap.data();
  const int map_stride = segmentsMap.stride();
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      const uint32_t label = map_line[x];
      ++components[label].size;
      boundingBoxes[label].extend(x, y);
    }
    map_line += map_stride;
  }

  // creating set of labels determining components to be removed
  std::unordered_set<uint32_t> labels;
  for (uint32_t label = 1; label <= segmentsMap.maxLabel(); ++label) {
    if (componentCleaner.eligibleForDelete(components[label], boundingBoxes[label])) {
      labels.insert(label);
    }
  }

  segmentsMap.removeComponents(labels);
}

QImage prepareToMap(const QImage& colorImage, const BinaryImage& image) {
  QImage dst = colorImage;
  applyMask(dst, image);
  return dst;
}

BinaryImage componentFromChannel(const QImage& colorImage, const RgbChannel channel, const int adjustment) {
  GrayImage channelImage = extractRgbChannel(colorImage, channel);
  BinaryThreshold threshold = BinaryThreshold::otsuThreshold(channelImage);
  BinaryImage component = BinaryImage(channelImage, adjustThreshold(threshold, adjustment));
  return component;
}

ConnectivityMap buildMapFromRgb(const BinaryImage& image,
                                const QImage& colorImage,
                                const Dpi& dpi,
                                const int noiseThreshold,
                                const int redThresholdAdjustment,
                                const int greenThresholdAdjustment,
                                const int blueThresholdAdjustment) {
  QImage imageToMap = prepareToMap(colorImage, image);

  BinaryImage redComponent = componentFromChannel(imageToMap, RED_CHANNEL, redThresholdAdjustment);
  BinaryImage greenComponent = componentFromChannel(imageToMap, GREEN_CHANNEL, greenThresholdAdjustment);
  BinaryImage blueComponent = componentFromChannel(imageToMap, BLUE_CHANNEL, blueThresholdAdjustment);

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

  ConnectivityMap segmentsMap = ConnectivityMap(blackComponent, CONN8);
  segmentsMap.addComponents(yellowComponent, CONN8);
  segmentsMap.addComponents(magentaComponent, CONN8);
  segmentsMap.addComponents(cyanComponent, CONN8);
  segmentsMap.addComponents(redComponent, CONN8);
  segmentsMap.addComponents(greenComponent, CONN8);
  segmentsMap.addComponents(blueComponent, CONN8);

  reduceNoise(segmentsMap, dpi, noiseThreshold);

  // Extend the map to cover unlabeled components.
  segmentsMap = InfluenceMap(segmentsMap, image);

  BinaryImage remainingComponents(image);
  rasterOp<RopSubtract<RopDst, RopSrc>>(remainingComponents, segmentsMap.getBinaryMask());
  segmentsMap.addComponents(remainingComponents, CONN8);

  return segmentsMap;
}

ConnectivityMap buildMapFromGrayscale(const BinaryImage& image) {
  return ConnectivityMap(image, CONN8);
}

class ComponentColor {
 public:
  inline void addPixelColor(uint32_t color) {
    m_red += qRed(color);
    m_green += qGreen(color);
    m_blue += qBlue(color);
    ++m_size;
  }

  inline uint32_t getColor() {
    const auto sizeF = static_cast<long double>(m_size);
    return qRgb(getAverage(m_red, m_size), getAverage(m_green, m_size), getAverage(m_blue, m_size));
  }

 private:
  static inline int getAverage(uint64_t sum, uint32_t size) {
    int average = sum / size;
    uint32_t reminder = sum % size;
    if (reminder >= (size - reminder)) {
      average++;
    }
    return average;
  }

  uint32_t m_size = 0;
  uint64_t m_red = 0;
  uint64_t m_green = 0;
  uint64_t m_blue = 0;
};

QImage buildRgbImage(const ConnectivityMap& segmentsMap, const QImage& colorImage) {
  const int width = colorImage.width();
  const int height = colorImage.height();

  std::vector<ComponentColor> compColorMap(segmentsMap.maxLabel() + 1);
  {
    const uint32_t* map_line = segmentsMap.data();
    const int map_stride = segmentsMap.stride();

    const auto* img_line = reinterpret_cast<const uint32_t*>(colorImage.bits());
    const int img_stride = colorImage.bytesPerLine() / sizeof(uint32_t);

    for (int y = 0; y < height; ++y) {
      for (int x = 0; x < width; ++x) {
        const uint32_t label = map_line[x];
        if (label == 0) {
          continue;
        }
        compColorMap[label].addPixelColor(img_line[x]);
      }
      map_line += map_stride;
      img_line += img_stride;
    }
  }

  QImage dst(colorImage.size(), QImage::Format_RGB32);
  dst.fill(Qt::white);

  auto* dst_line = reinterpret_cast<uint32_t*>(dst.bits());
  const int dst_stride = dst.bytesPerLine() / sizeof(uint32_t);

  const uint32_t* map_line = segmentsMap.data();
  const int map_stride = segmentsMap.stride();

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      const uint32_t label = map_line[x];
      if (label == 0) {
        continue;
      }
      dst_line[x] = compColorMap[label].getColor();
    }
    map_line += map_stride;
    dst_line += dst_stride;
  }

  return dst;
}

GrayImage buildGrayImage(const ConnectivityMap& segmentsMap, const GrayImage& grayImage) {
  const int width = grayImage.width();
  const int height = grayImage.height();

  std::vector<uint8_t> colorMap(segmentsMap.maxLabel() + 1, 0);

  {
    const uint32_t* map_line = segmentsMap.data();
    const int map_stride = segmentsMap.stride();

    const auto* img_line = grayImage.data();
    const int img_stride = grayImage.stride();

    std::vector<Component> components(segmentsMap.maxLabel() + 1);
    std::vector<uint32_t> graySumMap(segmentsMap.maxLabel() + 1, 0);
    for (int y = 0; y < height; ++y) {
      for (int x = 0; x < width; ++x) {
        const uint32_t label = map_line[x];
        if (label == 0) {
          continue;
        }

        ++components[label].size;
        graySumMap[label] += img_line[x];
      }
      map_line += map_stride;
      img_line += img_stride;
    }

    for (uint32_t label = 1; label <= segmentsMap.maxLabel(); label++) {
      colorMap[label] = static_cast<uint8_t>(std::round(double(graySumMap[label]) / components[label].size));
    }
  }

  GrayImage dst(grayImage.size());
  dst.fill(0xff);

  uint8_t* dst_line = dst.data();
  const int dst_stride = dst.stride();

  const uint32_t* map_line = segmentsMap.data();
  const int map_stride = segmentsMap.stride();

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
}  // namespace

QImage ColorSegmenter::segment(const BinaryImage& image, const QImage& colorImage) const {
  if (image.size() != colorImage.size()) {
    throw std::invalid_argument("ColorSegmenter: images size doesn't match.");
  }
  if ((colorImage.format() == QImage::Format_Indexed8) && colorImage.isGrayscale()) {
    return segment(image, GrayImage(colorImage));
  }
  if ((colorImage.format() != QImage::Format_RGB32) && (colorImage.format() != QImage::Format_ARGB32)) {
    throw std::invalid_argument("ColorSegmenter: wrong image format.");
  }

  ConnectivityMap segmentsMap = buildMapFromRgb(image, colorImage, m_dpi, m_noiseThreshold, m_redThresholdAdjustment,
                                                m_greenThresholdAdjustment, m_blueThresholdAdjustment);
  return buildRgbImage(segmentsMap, colorImage);
}

GrayImage ColorSegmenter::segment(const BinaryImage& image, const GrayImage& grayImage) const {
  if (image.size() != grayImage.size()) {
    throw std::invalid_argument("ColorSegmenter: images size doesn't match.");
  }

  ConnectivityMap segmentsMap = buildMapFromGrayscale(image);
  return buildGrayImage(segmentsMap, grayImage);
}
}  // namespace imageproc