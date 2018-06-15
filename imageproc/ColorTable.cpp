
#include "ColorTable.h"
#include <cassert>
#include <set>
#include <unordered_set>
#include "BinaryImage.h"

namespace imageproc {

ColorTable::ColorTable(const QImage& image) {
  if ((image.format() != QImage::Format_Indexed8) && (image.format() != QImage::Format_RGB32)
      && (image.format() != QImage::Format_ARGB32)) {
    throw std::invalid_argument("ColorTable: Image format not supported.");
  }

  this->image = image;
}

QImage ColorTable::getImage() const {
  return image;
}

QVector<QRgb> ColorTable::getPalette() const {
  std::unordered_map<uint32_t, int> paletteMap;
  switch (image.format()) {
    case QImage::Format_Indexed8:
      paletteMap = paletteFromIndexedWithStatistics();
      break;
    case QImage::Format_RGB32:
    case QImage::Format_ARGB32:
      paletteMap = paletteFromRgbWithStatistics();
      break;
    default:
      return QVector<QRgb>();
  }

  QVector<QRgb> palette(static_cast<int>(paletteMap.size()));
  for (const auto& colorAndStat : paletteMap) {
    palette.push_back(colorAndStat.first);
  }

  return palette;
}


ColorTable& ColorTable::posterize(const int level,
                                  bool normalize,
                                  const bool forceBlackAndWhite,
                                  const int normalizeBlackLevel,
                                  const int normalizeWhiteLevel) {
  if ((level < 2) || (level > 255)) {
    throw std::invalid_argument("ColorTable: level must be a value between 2 and 255 inclusive");
  }
  if ((level == 255) && !normalize && !forceBlackAndWhite) {
    return *this;
  }

  std::unordered_map<uint32_t, uint32_t> oldToNewColorMap;
  size_t newColorTableSize;

  {
    // Get the palette with statistics.
    std::unordered_map<uint32_t, int> paletteStatMap;
    switch (image.format()) {
      case QImage::Format_Indexed8:
        paletteStatMap = paletteFromIndexedWithStatistics();
        break;
      case QImage::Format_RGB32:
      case QImage::Format_ARGB32:
        paletteStatMap = paletteFromRgbWithStatistics();
        break;
      default:
        return *this;
    }

    // We have to normalize palette in order posterization to work with pale images.
    std::unordered_map<uint32_t, uint32_t> colorToNormalizedMap
        = normalizePalette(paletteStatMap, normalizeBlackLevel, normalizeWhiteLevel);

    // Build color groups resulted from splitting RGB space
    std::unordered_map<uint32_t, std::list<uint32_t>> groupMap;
    const double levelStride = 255.0 / level;
    for (const auto& colorAndStat : paletteStatMap) {
      const uint32_t color = colorAndStat.first;
      const uint32_t normalized_color = colorToNormalizedMap[color];

      auto redGroupIdx = static_cast<const int>(qRed(normalized_color) / levelStride);
      auto blueGroupIdx = static_cast<const int>(qGreen(normalized_color) / levelStride);
      auto greenGroupIdx = static_cast<const int>(qBlue(normalized_color) / levelStride);

      auto group = static_cast<uint32_t>((redGroupIdx << 16) | (greenGroupIdx << 8) | (blueGroupIdx));

      groupMap[group].push_back(color);
    }

    // Find the most often occurring color in the group and map the other colors in the group to that.
    for (const auto& groupAndColors : groupMap) {
      const std::list<uint32_t>& colors = groupAndColors.second;
      assert(!colors.empty());

      uint32_t mostOftenColorInGroup = *colors.begin();
      for (auto it = ++colors.begin(); it != colors.end(); ++it) {
        if (paletteStatMap[*it] > paletteStatMap[mostOftenColorInGroup]) {
          mostOftenColorInGroup = *it;
        }
      }

      if (forceBlackAndWhite) {
        if (normalize) {
          colorToNormalizedMap[0xff000000u] = 0xff000000u;
          colorToNormalizedMap[0xffffffffu] = 0xffffffffu;
        }
        makeGrayBlackOrWhiteInPlace(mostOftenColorInGroup, colorToNormalizedMap[mostOftenColorInGroup]);
      }

      for (const uint32_t& color : colors) {
        oldToNewColorMap[color] = normalize ? colorToNormalizedMap[mostOftenColorInGroup] : mostOftenColorInGroup;
      }
    }

    newColorTableSize = groupMap.size();
  }

  if (image.format() == QImage::Format_Indexed8) {
    remapColorsInIndexedImage(oldToNewColorMap);
  } else {
    if (newColorTableSize <= 256) {
      buildIndexedImageFromRgb(oldToNewColorMap);
    } else {
      remapColorsInRgbImage(oldToNewColorMap);
    }
  }

  return *this;
}

std::unordered_map<uint32_t, int> ColorTable::paletteFromIndexedWithStatistics() const {
  std::unordered_map<uint32_t, int> palette;

  const int width = image.width();
  const int height = image.height();

  const uint8_t* img_line = image.bits();
  const int img_stride = image.bytesPerLine();

  QVector<QRgb> colorTable = image.colorTable();

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      uint8_t colorIndex = img_line[x];
      uint32_t color = colorTable[colorIndex];

      ++palette[color];
    }
    img_line += img_stride;
  }

  return palette;
}

std::unordered_map<uint32_t, int> ColorTable::paletteFromRgbWithStatistics() const {
  std::unordered_map<uint32_t, int> palette;

  const int width = image.width();
  const int height = image.height();

  const auto* img_line = reinterpret_cast<const uint32_t*>(image.bits());
  const int img_stride = image.bytesPerLine() / sizeof(uint32_t);

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      uint32_t color = img_line[x];

      ++palette[color];
    }
    img_line += img_stride;
  }

  return palette;
}

void ColorTable::remapColorsInIndexedImage(const std::unordered_map<uint32_t, uint32_t>& colorMap) {
  std::unordered_map<uint32_t, uint8_t> colorToIndexMap;
  uint8_t index = 0;
  for (const auto& srcAndDstColors : colorMap) {
    if (colorToIndexMap.find(srcAndDstColors.second) == colorToIndexMap.end()) {
      colorToIndexMap[srcAndDstColors.second] = index++;
    }
  }

  {
    const int width = image.width();
    const int height = image.height();

    uint8_t* img_line = image.bits();
    const int img_stride = image.bytesPerLine();

    QVector<QRgb> colorTable = image.colorTable();

    for (int y = 0; y < height; ++y) {
      for (int x = 0; x < width; ++x) {
        uint32_t color = colorTable[img_line[x]];
        uint32_t newColor = colorMap.at(color);
        img_line[x] = colorToIndexMap[newColor];
      }
      img_line += img_stride;
    }
  }

  QVector<QRgb> newColorTable(static_cast<int>(colorToIndexMap.size()));
  for (const auto& colorAndIndex : colorToIndexMap) {
    newColorTable[colorAndIndex.second] = colorAndIndex.first;
  }

  image.setColorTable(newColorTable);
}

void ColorTable::remapColorsInRgbImage(const std::unordered_map<uint32_t, uint32_t>& colorMap) {
  const int width = image.width();
  const int height = image.height();

  auto* img_line = reinterpret_cast<uint32_t*>(image.bits());
  const int img_stride = image.bytesPerLine() / sizeof(uint32_t);

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      uint32_t color = img_line[x];
      img_line[x] = colorMap.at(color);
    }
    img_line += img_stride;
  }
}

void ColorTable::buildIndexedImageFromRgb(const std::unordered_map<uint32_t, uint32_t>& colorMap) {
  remapColorsInRgbImage(colorMap);

  std::unordered_set<uint32_t> colorSet;
  for (const auto& srcAndDstColors : colorMap) {
    colorSet.insert(srcAndDstColors.second);
  }

  QVector<QRgb> newColorTable(static_cast<int>(colorSet.size()));
  for (const auto& color : colorSet) {
    newColorTable.push_back(color);
  }

  image = toIndexedImage(&newColorTable);
}

std::unordered_map<uint32_t, uint32_t> ColorTable::normalizePalette(const std::unordered_map<uint32_t, int>& palette,
                                                                    const int normalizeBlackLevel,
                                                                    const int normalizeWhiteLevel) const {
  const int pixelCount = image.width() * image.height();
  const double threshold = 0.0005;  // mustn't be larger than (1 / 256)

  int min_level = 255;
  int max_level = 0;
  {
    // Build RGB histogram from colors with statistics
    int red_hist[256] = {};
    int green_hist[256] = {};
    int blue_hist[256] = {};
    for (const auto& colorAndStat : palette) {
      const uint32_t color = colorAndStat.first;
      const int statistics = colorAndStat.second;

      if (color == 0xff000000u) {
        red_hist[normalizeBlackLevel] += statistics;
        green_hist[normalizeBlackLevel] += statistics;
        blue_hist[normalizeBlackLevel] += statistics;
        continue;
      }
      if (color == 0xffffffffu) {
        red_hist[normalizeWhiteLevel] += statistics;
        green_hist[normalizeWhiteLevel] += statistics;
        blue_hist[normalizeWhiteLevel] += statistics;
        continue;
      }

      red_hist[qRed(color)] += statistics;
      green_hist[qGreen(color)] += statistics;
      blue_hist[qBlue(color)] += statistics;
    }

    // Find the max and min levels discarding a noise
    for (int level = 0; level < 256; ++level) {
      if (((double(red_hist[level]) / pixelCount) >= threshold)
          || ((double(green_hist[level]) / pixelCount) >= threshold)
          || ((double(blue_hist[level]) / pixelCount) >= threshold)) {
        if (level < min_level) {
          min_level = level;
        }
        if (level > max_level) {
          max_level = level;
        }
      }
    }

    assert(max_level >= min_level);
  }

  std::unordered_map<uint32_t, uint32_t> colorToNormalizedMap;
  for (const auto& colorAndStat : palette) {
    const uint32_t color = colorAndStat.first;
    if (color == 0xff000000u) {
      colorToNormalizedMap[0xff000000u] = 0xff000000u;
      continue;
    }
    if (color == 0xffffffffu) {
      colorToNormalizedMap[0xffffffffu] = 0xffffffffu;
      continue;
    }

    int normalizedRed = qRound((double(qRed(color) - min_level) / (max_level - min_level)) * 255);
    int normalizedGreen = qRound((double(qGreen(color) - min_level) / (max_level - min_level)) * 255);
    int normalizedBlue = qRound((double(qBlue(color) - min_level) / (max_level - min_level)) * 255);
    normalizedRed = qBound(0, normalizedRed, 255);
    normalizedGreen = qBound(0, normalizedGreen, 255);
    normalizedBlue = qBound(0, normalizedBlue, 255);

    colorToNormalizedMap[color] = qRgb(normalizedRed, normalizedGreen, normalizedBlue);
  }

  return colorToNormalizedMap;
}

namespace {
bool isGray(const QColor& color) {
  const double saturation = color.saturationF();
  const double value = color.valueF();

  const double coefficient = std::max(.0, ((std::max(saturation, value) - 0.28) / 0.72)) + 1;

  return (saturation * value) < (0.1 * coefficient);
}
}  // namespace

void ColorTable::makeGrayBlackOrWhiteInPlace(QRgb& rgb, const QRgb& normalized) const {
  const QColor color = QColor(normalized).toHsv();

  if (isGray(color)) {
    const int grayLevel = qGray(normalized);
    const QColor grayColor = QColor(grayLevel, grayLevel, grayLevel).toHsl();
    if (grayColor.lightnessF() <= 0.5) {
      rgb = 0xff000000u;
    } else if (grayColor.lightnessF() >= 0.8) {
      rgb = 0xffffffffu;
    }
  }
}

QImage ColorTable::toIndexedImage(const QVector<QRgb>* colorTable) const {
  if (image.format() == QImage::Format_Indexed8) {
    return image;
  }

  const QVector<QRgb>& palette = (colorTable) ? *colorTable : getPalette();
  if (palette.size() > 256) {
    return image;
  }

  QImage dst(image.size(), QImage::Format_Indexed8);
  dst.setColorTable(palette);

  const int width = image.width();
  const int height = image.height();

  const auto* img_line = reinterpret_cast<const uint32_t*>(image.bits());
  const int img_stride = image.bytesPerLine() / sizeof(uint32_t);

  uint8_t* dst_line = dst.bits();
  const int dst_stride = dst.bytesPerLine();

  std::unordered_map<uint32_t, uint8_t> colorToIndex;
  for (int i = 0; i < palette.size(); ++i) {
    colorToIndex[palette[i]] = static_cast<uint8_t>(i);
  }

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      dst_line[x] = colorToIndex[img_line[x]];
    }
    img_line += img_stride;
    dst_line += dst_stride;
  }

  dst.setDotsPerMeterX(image.dotsPerMeterX());
  dst.setDotsPerMeterY(image.dotsPerMeterY());

  return dst;
}

}  // namespace imageproc