
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

        template<typename MixedPixel>
        void applyMask(QImage& image, const BinaryImage& bw_mask) {
            MixedPixel* image_line = reinterpret_cast<MixedPixel*>(image.bits());
            const int image_stride = image.bytesPerLine() / sizeof(MixedPixel);
            const uint32_t* bw_mask_line = bw_mask.data();
            const int bw_mask_stride = bw_mask.wordsPerLine();
            const int width = image.width();
            const int height = image.height();
            const uint32_t msb = uint32_t(1) << 31;
            const MixedPixel fillingPixel = static_cast<MixedPixel>(0xffffffff);

            for (int y = 0; y < height; ++y) {
                for (int x = 0; x < width; ++x) {
                    if (!(bw_mask_line[x >> 5] & (msb >> (x & 31)))) {
                        image_line[x] = fillingPixel;
                    }
                }
                image_line += image_stride;
                bw_mask_line += bw_mask_stride;
            }
        }
    } // namespace

    SplitImage::SplitImage() = default;

    SplitImage::SplitImage(const QImage& foreground, const QImage& background) {
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

        if (!mask.isNull()) {
            return backgroundImage;
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

    QImage SplitImage::getForegroundImage() const {
        if (!mask.isNull()) {
            QImage foreground(backgroundImage);
            if (backgroundImage.format() == QImage::Format_Indexed8) {
                applyMask<uint8_t>(foreground, mask);
            } else {
                applyMask<uint32_t>(foreground, mask);
            }
            if (binaryForeground) {
                foreground = foreground.convertToFormat(QImage::Format_Mono);
            }

            return foreground;
        }

        return foregroundImage;
    }

    void SplitImage::setForegroundImage(const QImage& foregroundImage) {
        mask = BinaryImage();
        SplitImage::foregroundImage = foregroundImage;
    }

    QImage SplitImage::getBackgroundImage() const {
        if (!mask.isNull()) {
            QImage background(backgroundImage);
            if (backgroundImage.format() == QImage::Format_Indexed8) {
                applyMask<uint8_t>(background, mask.inverted());
            } else {
                applyMask<uint32_t>(background, mask.inverted());
            }

            return background;
        }

        return backgroundImage;
    }

    void SplitImage::setBackgroundImage(const QImage& backgroundImage) {
        SplitImage::backgroundImage = backgroundImage;
    }

    void SplitImage::applyToLayerImages(std::function<void(QImage&)> function) {
        if (!foregroundImage.isNull()) {
            function(foregroundImage);
        }
        if (!backgroundImage.isNull()) {
            function(backgroundImage);
        }
    }

    bool SplitImage::isNull() const {
        return (foregroundImage.isNull() && mask.isNull())
               || backgroundImage.isNull();
    }

    void SplitImage::setMask(const BinaryImage& mask, bool binaryForeground) {
        foregroundImage = QImage();
        SplitImage::mask = mask;
        SplitImage::binaryForeground = binaryForeground;
    }
} // namespace output
