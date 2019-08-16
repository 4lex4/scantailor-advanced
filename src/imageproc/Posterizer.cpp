
#include "Posterizer.h"
#include <cassert>
#include <set>
#include <unordered_set>
#include "BinaryImage.h"

namespace imageproc {
Posterizer::Posterizer(int level,
                       bool normalize,
                       bool forceBlackAndWhite,
                       int normalizeBlackLevel,
                       int normalizeWhiteLevel)
    : m_level(level),
      m_normalize(normalize),
      m_forceBlackAndWhite(forceBlackAndWhite),
      m_normalizeBlackLevel(normalizeBlackLevel),
      m_normalizeWhiteLevel(normalizeWhiteLevel) {
  if ((level < 2) || (level > 255)) {
    throw std::invalid_argument("Posterizer: level must be a value between 2 and 255 inclusive");
  }
}

namespace {
QVector<QRgb> paletteFromRgb(const QImage& image) {
  std::unordered_set<uint32_t> colorSet;

  const int width = image.width();
  const int height = image.height();

  const auto* img_line = reinterpret_cast<const uint32_t*>(image.bits());
  const int img_stride = image.bytesPerLine() / sizeof(uint32_t);

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      uint32_t color = img_line[x];
      colorSet.insert(color);
    }
    img_line += img_stride;
  }

  QVector<QRgb> palette(colorSet.size());
  std::copy(colorSet.begin(), colorSet.end(), std::back_inserter(palette));

  return palette;
}
}  // namespace

QVector<QRgb> Posterizer::buildPalette(const QImage& image) {
  std::unordered_map<uint32_t, int> paletteMap;
  switch (image.format()) {
    case QImage::Format_Indexed8:
      return image.colorTable();
    case QImage::Format_RGB32:
    case QImage::Format_ARGB32:
      return paletteFromRgb(image);
    default:
      throw std::invalid_argument("Posterizer::buildPalette(): invalid image format");
  }
}

QImage Posterizer::convertToIndexed(const QImage& image) {
  if (image.format() == QImage::Format_Indexed8) {
    return image;
  }
  return convertToIndexed(image, buildPalette(image));
}

QImage Posterizer::convertToIndexed(const QImage& image, const QVector<QRgb>& palette) {
  if (image.format() == QImage::Format_Indexed8) {
    return image;
  }
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

namespace {
std::unordered_map<uint32_t, int> paletteFromIndexedWithStatistics(const QImage& image) {
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

std::unordered_map<uint32_t, int> paletteFromRgbWithStatistics(const QImage& image) {
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

void remapColorsInIndexedImage(QImage& image, const std::unordered_map<uint32_t, uint32_t>& colorMap) {
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

  QVector<QRgb> newColorTable(colorToIndexMap.size());
  for (const auto& colorAndIndex : colorToIndexMap) {
    newColorTable[colorAndIndex.second] = colorAndIndex.first;
  }

  image.setColorTable(newColorTable);
}

void remapColorsInRgbImage(QImage& image, const std::unordered_map<uint32_t, uint32_t>& colorMap) {
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

QVector<QRgb> paletteFromColorMap(const std::unordered_map<uint32_t, uint32_t>& colorMap) {
  std::unordered_set<uint32_t> colorSet;
  for (const auto& srcAndDstColors : colorMap) {
    colorSet.insert(srcAndDstColors.second);
  }

  QVector<QRgb> palette(static_cast<int>(colorSet.size()));
  std::copy(colorSet.begin(), colorSet.end(), std::back_inserter(palette));

  return palette;
}

std::unordered_map<uint32_t, uint32_t> normalizePalette(const QImage& image,
                                                        const std::unordered_map<uint32_t, int>& palette,
                                                        const int normalizeBlackLevel = 0,
                                                        const int normalizeWhiteLevel = 255) {
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

bool isGray(const QColor& color) {
  const double saturation = color.saturationF();
  const double value = color.valueF();

  const double coefficient = std::max(.0, ((std::max(saturation, value) - 0.28) / 0.72)) + 1;

  return (saturation * value) < (0.1 * coefficient);
}

void makeGrayBlackOrWhiteInPlace(QRgb& rgb, const QRgb& normalized) {
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
}  // namespace

QImage Posterizer::posterize(const QImage& image) const {
  if ((m_level == 255) && !m_normalize && !m_forceBlackAndWhite) {
    return image;
  }

  std::unordered_map<uint32_t, uint32_t> oldToNewColorMap;
  size_t newColorTableSize;

  {
    // Get the palette with statistics.
    std::unordered_map<uint32_t, int> paletteStatMap;
    switch (image.format()) {
      case QImage::Format_Indexed8:
        paletteStatMap = paletteFromIndexedWithStatistics(image);
        break;
      case QImage::Format_RGB32:
      case QImage::Format_ARGB32:
        paletteStatMap = paletteFromRgbWithStatistics(image);
        break;
      default:
        throw std::invalid_argument("Posterizer: invalid image format");
    }

    // We have to normalize palette in order posterization to work with pale images.
    std::unordered_map<uint32_t, uint32_t> colorToNormalizedMap
        = normalizePalette(image, paletteStatMap, m_normalizeBlackLevel, m_normalizeWhiteLevel);

    // Build color groups resulted from splitting RGB space
    std::unordered_map<uint32_t, std::list<uint32_t>> groupMap;
    const double levelStride = 255.0 / m_level;
    for (const auto& colorAndStat : paletteStatMap) {
      const uint32_t color = colorAndStat.first;
      const uint32_t normalized_color = colorToNormalizedMap[color];

      const auto redGroupIdx = static_cast<int>(qRed(normalized_color) / levelStride);
      const auto blueGroupIdx = static_cast<int>(qGreen(normalized_color) / levelStride);
      const auto greenGroupIdx = static_cast<int>(qBlue(normalized_color) / levelStride);

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

      if (m_forceBlackAndWhite) {
        if (m_normalize) {
          colorToNormalizedMap[0xff000000u] = 0xff000000u;
          colorToNormalizedMap[0xffffffffu] = 0xffffffffu;
        }
        makeGrayBlackOrWhiteInPlace(mostOftenColorInGroup, colorToNormalizedMap[mostOftenColorInGroup]);
      }

      for (const uint32_t& color : colors) {
        oldToNewColorMap[color] = m_normalize ? colorToNormalizedMap[mostOftenColorInGroup] : mostOftenColorInGroup;
      }
    }

    newColorTableSize = groupMap.size();
  }

  QImage dst(image);
  if (dst.format() == QImage::Format_Indexed8) {
    remapColorsInIndexedImage(dst, oldToNewColorMap);
  } else {
    remapColorsInRgbImage(dst, oldToNewColorMap);
    if (newColorTableSize <= 256) {
      return Posterizer::convertToIndexed(dst, paletteFromColorMap(oldToNewColorMap));
    }
  }

  return dst;
}
}  // namespace imageproc