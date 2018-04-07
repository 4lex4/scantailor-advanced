
#ifndef SCANTAILOR_BACKGROUNDCOLORCALCULATOR_H
#define SCANTAILOR_BACKGROUNDCOLORCALCULATOR_H


#include <cstdint>

class QImage;
class QColor;
class QPolygonF;
class DebugImages;

namespace imageproc {
class GrayscaleHistogram;
class BinaryImage;

class BackgroundColorCalculator {
public:
    static QColor calcDominantBackgroundColor(const QImage& img);

    static QColor calcDominantBackgroundColor(const QImage& img, const BinaryImage& mask, DebugImages* dbg = nullptr);

    static QColor calcDominantBackgroundColor(const QImage& img,
                                              const QPolygonF& crop_area,
                                              DebugImages* dbg = nullptr);

private:
    static uint8_t calcDominantLevel(const int* hist);

    static QColor calcDominantColor(const QImage& img, const BinaryImage& background_mask);
};
}  // namespace imageproc


#endif  // SCANTAILOR_BACKGROUNDCOLORCALCULATOR_H
