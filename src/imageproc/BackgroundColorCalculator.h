
#ifndef SCANTAILOR_BACKGROUNDCOLORCALCULATOR_H
#define SCANTAILOR_BACKGROUNDCOLORCALCULATOR_H

#include <cstdint>

class QImage;
class QColor;
class QPolygonF;

namespace imageproc {
class GrayscaleHistogram;
class BinaryImage;

class BackgroundColorCalculator {
 public:
  static QColor calcDominantBackgroundColor(const QImage& img);

  static QColor calcDominantBackgroundColor(const QImage& img, const BinaryImage& mask);

  static QColor calcDominantBackgroundColor(const QImage& img, const QPolygonF& crop_area);

 private:
  static uint8_t calcDominantLevel(const int* hist);

  static QColor calcDominantColor(const QImage& img, const BinaryImage& background_mask);
};
}  // namespace imageproc


#endif  // SCANTAILOR_BACKGROUNDCOLORCALCULATOR_H
