
#ifndef SCANTAILOR_COLORSEGMENTER_H
#define SCANTAILOR_COLORSEGMENTER_H

#include <QImage>
#include "BinaryThreshold.h"
#include "ConnectivityMap.h"

class Dpi;
class QRect;

namespace imageproc {
class BinaryImage;
class GrayImage;

class ColorSegmenter {
 private:
  struct Component;
  struct BoundingBox;

  enum RgbChannel { RED_CHANNEL, GREEN_CHANNEL, BLUE_CHANNEL };

  class Settings {
   private:
    /**
     * Defines the minimum average width threshold.
     * When a component has lower that, it will be erased.
     */
    double minAverageWidthThreshold;

    /**
     * Defines the minimum square in pixels.
     * If a component has lower that, it will be erased.
     */
    int bigObjectThreshold;

   public:
    explicit Settings(const Dpi& dpi, int noiseThreshold);

    bool eligibleForDelete(const Component& component, const BoundingBox& boundingBox) const;
  };

 public:
  ColorSegmenter(const BinaryImage& image,
                 const QImage& originalImage,
                 const Dpi& dpi,
                 int noiseThreshold,
                 int redThresholdAdjustment,
                 int greenThresholdAdjustment,
                 int blueThresholdAdjustment);

  ColorSegmenter(const BinaryImage& image, const GrayImage& originalImage, const Dpi& dpi, int noiseThreshold);

  QImage getImage() const;

 private:
  static GrayImage getRgbChannel(const QImage& image, RgbChannel channel);

  void reduceNoise();

  void fromRgb(const BinaryImage& image,
               const QImage& originalImage,
               int redThresholdAdjustment,
               int greenThresholdAdjustment,
               int blueThresholdAdjustment);

  void fromGrayscale(const BinaryImage& image, const GrayImage& originalImage);

  QImage buildRgbImage() const;

  QImage buildGrayImage() const;

  BinaryThreshold adjustThreshold(BinaryThreshold threshold, int adjustment);

  Settings settings;
  ConnectivityMap segmentsMap;
  QImage originalImage;
};
}  // namespace imageproc

#endif  // SCANTAILOR_COLORSEGMENTER_H
