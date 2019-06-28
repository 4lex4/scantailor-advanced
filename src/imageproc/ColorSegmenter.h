
#ifndef SCANTAILOR_COLORSEGMENTER_H
#define SCANTAILOR_COLORSEGMENTER_H

#include <QImage>
#include "Dpi.h"

namespace imageproc {
class BinaryImage;
class GrayImage;

class ColorSegmenter {
 public:
  ColorSegmenter(const Dpi& dpi,
                 int noiseThreshold,
                 int redThresholdAdjustment,
                 int greenThresholdAdjustment,
                 int blueThresholdAdjustment);

  ColorSegmenter(const Dpi& dpi, int noiseThreshold);

  QImage segment(const BinaryImage& image, const QImage& colorImage) const;

  GrayImage segment(const BinaryImage& image, const GrayImage& grayImage) const;

 private:
  Dpi m_dpi;
  int m_noiseThreshold;
  int m_redThresholdAdjustment;
  int m_greenThresholdAdjustment;
  int m_blueThresholdAdjustment;
};
}  // namespace imageproc

#endif  // SCANTAILOR_COLORSEGMENTER_H
