
#ifndef SCANTAILOR_COLORTABLE_H
#define SCANTAILOR_COLORTABLE_H


#include <QtGui/QImage>
#include <unordered_map>

namespace imageproc {
    class ColorTable {
    private:
        QImage image;

    public:
        explicit ColorTable(const QImage& image);

        ColorTable& posterize(int level, bool forceBlackAndWhite = false);

        QVector<QRgb> getPalette() const;

        QImage getImage() const;

    private:
        std::unordered_map<uint32_t, int> paletteFromMonoWithStatistics() const;

        std::unordered_map<uint32_t, int> paletteFromIndexedWithStatistics() const;

        std::unordered_map<uint32_t, int> paletteFromRgbWithStatistics() const;

        void remapColorsInIndexedImage(const std::unordered_map<uint32_t, uint32_t>& colorMap);

        void remapColorsInRgbImage(const std::unordered_map<uint32_t, uint32_t>& colorMap);

        void buildIndexedImageFromRgb(const std::unordered_map<uint32_t, uint32_t>& colorMap);
    };
}

#endif //SCANTAILOR_COLORTABLE_H
