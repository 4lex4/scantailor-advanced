
#ifndef SCANTAILOR_BACKGROUNDCOLORCALCULATOR_H
#define SCANTAILOR_BACKGROUNDCOLORCALCULATOR_H


#include <cstdint>
#include "BinaryImage.h"

class QImage;
class QColor;
class QPolygonF;
class DebugImages;

namespace imageproc {
class GrayscaleHistogram;

class BackgroundColorCalculator {
 public:
  explicit BackgroundColorCalculator(bool internalBlackOnWhiteDetection = true);

  QColor calcDominantBackgroundColor(const QImage& img) const;

  QColor calcDominantBackgroundColor(const QImage& img, const BinaryImage& mask, DebugImages* dbg = nullptr) const;

  QColor calcDominantBackgroundColor(const QImage& img, const QPolygonF& crop_area, DebugImages* dbg = nullptr) const;

 private:
  static uint8_t calcDominantLevel(const int* hist);

  static QColor calcDominantColor(const QImage& img, const BinaryImage& background_mask);

  bool isBlackOnWhite(const BinaryImage& img) const;

  bool isBlackOnWhite(BinaryImage img, const BinaryImage& mask) const;


  bool internalBlackOnWhiteDetection;
};
}  // namespace imageproc


#endif  // SCANTAILOR_BACKGROUNDCOLORCALCULATOR_H
