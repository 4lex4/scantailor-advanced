
#ifndef SCANTAILOR_COLORSEGMENTER_H
#define SCANTAILOR_COLORSEGMENTER_H

#include "ConnectivityMap.h"
#include <QImage>

class Dpi;
class QRect;

namespace imageproc {
    class BinaryImage;
    class GrayImage;

    class ColorSegmenter {
    private:
        struct Component;
        struct BoundingBox;

        enum RgbChannel {
            RED_CHANNEL,
            GREEN_CHANNEL,
            BLUE_CHANNEL
        };

        class Settings {
        private:
            /**
             * Defines the minimum average width threshold.
             * When a component has lower that, it will be erased.
             */
            double minAverageWidthThreshold;

            /**
             * Defines the minimum square in pixels that will guarantee
             * the object won't be removed.
             */
            int bigObjectThreshold;

        public:
            explicit Settings(const Dpi& dpi);

            bool eligibleForDelete(const Component& component, const BoundingBox& boundingBox) const;
        };

    public:
        ColorSegmenter(const BinaryImage& image, const QImage& originalImage, const Dpi& dpi);

        ColorSegmenter(const BinaryImage& image, const GrayImage& originalImage, const Dpi& dpi);

        QImage getImage() const;

    private:

        static GrayImage getRgbChannel(const QImage& image, RgbChannel channel);

        void reduceNoise();

        void fromRgb(const BinaryImage& image, const QImage& originalImage);

        void fromGrayscale(const BinaryImage& image, const GrayImage& originalImage);

        QImage buildRgbImage() const;

        QImage buildGrayImage() const;

        Settings settings;
        ConnectivityMap segmentsMap;
        QImage originalImage;
    };
}

#endif //SCANTAILOR_COLORSEGMENTER_H
