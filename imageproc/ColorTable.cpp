
#include <cassert>
#include <unordered_set>
#include "ColorTable.h"
#include "BinaryImage.h"

namespace imageproc {

    ColorTable::ColorTable(const QImage& image) {
        if ((image.format() != QImage::Format_Mono)
            && (image.format() != QImage::Format_MonoLSB)
            && (image.format() != QImage::Format_Indexed8)
            && (image.format() != QImage::Format_RGB32)
            && (image.format() != QImage::Format_ARGB32)) {
            throw std::invalid_argument("Image format not supported.");
        }

        this->image = image;
    }

    QImage ColorTable::getImage() const {
        return image;
    }

    QVector<QRgb> ColorTable::getPalette() const {
        std::unordered_map<uint32_t, int> paletteMap;
        switch (image.format()) {
            case QImage::Format_Mono:
            case QImage::Format_MonoLSB:
                paletteMap = paletteFromMonoWithStatistics();
                break;
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


    ColorTable& ColorTable::posterize(const int level, const bool forceBlackAndWhite) {
        if ((level < 2) || (level > 255)) {
            throw std::invalid_argument("error: level must be a value between 2 and 255 inclusive");
        }
        if (level == 255) {
            return *this;
        }

        std::unordered_map<uint32_t, uint32_t> oldToNewColorMap;
        size_t newColorsCount;

        {
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

            std::unordered_map<uint32_t, std::list<uint32_t>> groupMap;
            const double levelStride = 255.0 / level;
            for (const auto& colorAndStat : paletteStatMap) {
                const uint32_t color = colorAndStat.first;

                auto redGroupIdx = static_cast<const int>(qRed(color) / levelStride);
                auto blueGroupIdx = static_cast<const int>(qGreen(color) / levelStride);
                auto greenGroupIdx = static_cast<const int>(qBlue(color) / levelStride);

                auto group = static_cast<uint32_t>((redGroupIdx << 16) | (greenGroupIdx << 8) | (blueGroupIdx));
                groupMap[group].push_back(color);
            }

            if (forceBlackAndWhite) {
                const int maxLevelIdx = level - 1;
                auto whiteGroup = static_cast<uint32_t>((maxLevelIdx << 16) | (maxLevelIdx << 8) | (maxLevelIdx));
                uint32_t blackGroup = 0;

                groupMap[whiteGroup].push_back(0xffffffffu);
                groupMap[blackGroup].push_back(0xff000000u);

                paletteStatMap[0xffffffffu] = std::numeric_limits<int>::max();
                paletteStatMap[0xff000000u] = std::numeric_limits<int>::max();
            }

            for (const auto& groupAndColors : groupMap) {
                const std::list<uint32_t>& colors = groupAndColors.second;
                assert(!colors.empty());

                uint32_t mostOftenColorInGroup = *colors.begin();
                for (auto it = ++colors.begin(); it != colors.end(); ++it) {
                    if (paletteStatMap[*it] > paletteStatMap[mostOftenColorInGroup]) {
                        mostOftenColorInGroup = *it;
                    }
                }

                for (uint32_t color : colors) {
                    oldToNewColorMap[color] = mostOftenColorInGroup;
                }
            }

            newColorsCount = groupMap.size();
        }

        if (image.format() == QImage::Format_Indexed8) {
            remapColorsInIndexedImage(oldToNewColorMap);
        } else {
            if (newColorsCount <= 256) {
                buildIndexedImageFromRgb(oldToNewColorMap);
            } else {
                remapColorsInRgbImage(oldToNewColorMap);
            }
        }

        return *this;
    }

    std::unordered_map<uint32_t, int> ColorTable::paletteFromMonoWithStatistics() const {
        std::unordered_map<uint32_t, int> palette;

        BinaryImage bwImage(image);

        const int allCount = bwImage.size().width() * bwImage.size().height();
        const int blackCount = bwImage.countBlackPixels();
        const int whiteCount = allCount - blackCount;

        if (blackCount != 0) {
            palette[0xff000000u] = blackCount;
        }
        if (whiteCount != 0) {
            palette[0xffffffffu] = whiteCount;
        }

        return palette;
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

        image = image.convertToFormat(QImage::Format_Indexed8, newColorTable);
    }

}  // namespace imageproc