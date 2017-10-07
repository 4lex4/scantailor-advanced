
#include <imageproc/BinaryImage.h>
#include "SplitImage.h"

using namespace imageproc;

namespace output {
    namespace {
        template<typename MixedPixel>
        void combineImageMono(QImage& mixedImage,
                              BinaryImage const& foreground) {
            MixedPixel* mixed_line = reinterpret_cast<MixedPixel*>(mixedImage.bits());
            const int mixed_stride = mixedImage.bytesPerLine() / sizeof(MixedPixel);
            const uint32_t* foreground_line = foreground.data();
            const int foreground_stride = foreground.wordsPerLine();
            const int width = mixedImage.width();
            const int height = mixedImage.height();
            const uint32_t msb = uint32_t(1) << 31;

            for (int y = 0; y < height; ++y) {
                for (int x = 0; x < width; ++x) {
                    if (foreground_line[x >> 5] & (msb >> (x & 31))) {
                        uint32_t tmp = foreground_line[x >> 5];
                        tmp >>= (31 - (x & 31));
                        tmp &= uint32_t(1);

                        --tmp;
                        tmp |= 0xff000000;
                        mixed_line[x] = static_cast<MixedPixel>(tmp);
                    }
                }
                mixed_line += mixed_stride;
                foreground_line += foreground_stride;
            }
        }

        template<typename MixedPixel>
        void combineImageColor(QImage& mixedImage,
                               QImage const& foreground) {
            MixedPixel* mixed_line = reinterpret_cast<MixedPixel*>(mixedImage.bits());
            const int mixed_stride = mixedImage.bytesPerLine() / sizeof(MixedPixel);
            const MixedPixel* foreground_line = reinterpret_cast<const MixedPixel*>(foreground.bits());
            const int foreground_stride = foreground.bytesPerLine() / sizeof(MixedPixel);
            const int width = mixedImage.width();
            const int height = mixedImage.height();
            const uint32_t msb = uint32_t(0x00ffffff);

            for (int y = 0; y < height; ++y) {
                for (int x = 0; x < width; ++x) {
                    if ((foreground_line[x] & msb) != msb) {
                        mixed_line[x] = foreground_line[x];
                    }
                }
                mixed_line += mixed_stride;
                foreground_line += foreground_stride;
            }
        }
    } // namespace

    SplitImage::SplitImage()
            : foregroundImage(QImage()),
              backgroundImage(QImage()) {
    }

    SplitImage::SplitImage(const QImage& foreground, const QImage& background)
            : foregroundImage(QImage()),
              backgroundImage(QImage()) {
        if ((foreground.format() != QImage::Format_Mono)
            && (foreground.format() != QImage::Format_MonoLSB)
            && (foreground.format() != QImage::Format_Indexed8)
            && (foreground.format() != QImage::Format_RGB32)
            && (foreground.format() != QImage::Format_ARGB32)) {
            return;
        }
        if ((background.format() != QImage::Format_Indexed8)
            && (background.format() != QImage::Format_RGB32)
            && (background.format() != QImage::Format_ARGB32)) {
            return;
        }
        if (!((foreground.format() == QImage::Format_Mono)
              || (foreground.format() == QImage::Format_MonoLSB))
            && (foreground.format() != background.format())) {
            return;
        }

        if (foreground.size() != background.size()) {
            return;
        }

        foregroundImage = foreground;
        backgroundImage = background;
    }

    QImage SplitImage::toImage() const {
        if (isNull()) {
            return QImage();
        }

        QImage dst(backgroundImage);
        if ((foregroundImage.format() == QImage::Format_Mono)
            || (foregroundImage.format() == QImage::Format_MonoLSB)) {
            if (backgroundImage.format() == QImage::Format_Indexed8) {
                combineImageMono<uint8_t>(dst, BinaryImage(foregroundImage));
            } else {
                combineImageMono<uint32_t>(dst, BinaryImage(foregroundImage));
            }
        } else {
            if (backgroundImage.format() == QImage::Format_Indexed8) {
                combineImageColor<uint8_t>(dst, foregroundImage);
            } else {
                combineImageColor<uint32_t>(dst, foregroundImage);
            }
        }
        
        return dst;
    }

    const QImage& SplitImage::getForegroundImage() const {
        return foregroundImage;
    }

    void SplitImage::setForegroundImage(const QImage& foregroundImage) {
        SplitImage::foregroundImage = foregroundImage;
    }

    const QImage& SplitImage::getBackgroundImage() const {
        return backgroundImage;
    }

    void SplitImage::setBackgroundImage(const QImage& backgroundImage) {
        SplitImage::backgroundImage = backgroundImage;
    }

    void SplitImage::applyToLayerImages(std::function<void(QImage&)> function) {
        function(foregroundImage);
        function(backgroundImage);
    }

    bool SplitImage::isNull() const {
        return (foregroundImage.isNull() || backgroundImage.isNull());
    }
} // namespace output
