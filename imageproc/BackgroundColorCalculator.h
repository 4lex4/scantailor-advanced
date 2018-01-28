
#ifndef SCANTAILOR_BACKGROUNDCOLORCALCULATOR_H
#define SCANTAILOR_BACKGROUNDCOLORCALCULATOR_H


#include <cstdint>

class QImage;
class QColor;

namespace imageproc {
    class GrayscaleHistogram;
    class BinaryImage;

    class BackgroundColorCalculator {
    public:
        static QColor calcDominantBackgroundColor(const QImage& img);

        static QColor calcDominantBackgroundColorBW(const QImage& img);

    private:
        static uint8_t calcDominantLevel(const int* hist);
    };
}


#endif //SCANTAILOR_BACKGROUNDCOLORCALCULATOR_H
