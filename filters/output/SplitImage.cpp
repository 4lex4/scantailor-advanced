
#include <cassert>
#include <imageproc/RasterOp.h>
#include "SplitImage.h"

using namespace imageproc;

namespace output {
    namespace {
        template<typename MixedPixel>
        void combineImageMono(QImage& mixedImage,
                              BinaryImage const& foreground) {
            auto* mixed_line = reinterpret_cast<MixedPixel*>(mixedImage.bits());
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

        void combineImageMono(QImage& mixedImage,
                              BinaryImage const& foreground) {
            if ((mixedImage.format() != QImage::Format_Indexed8)
                && (mixedImage.format() != QImage::Format_RGB32)
                && (mixedImage.format() != QImage::Format_ARGB32)) {
                throw std::invalid_argument("Error: wrong image format.");
            }

            if (mixedImage.format() == QImage::Format_Indexed8) {
                combineImageMono<uint8_t>(mixedImage, foreground);
            } else {
                combineImageMono<uint32_t>(mixedImage, foreground);
            }
        }

        template<typename MixedPixel>
        void combineImageMono(QImage& mixedImage,
                              BinaryImage const& foreground,
                              BinaryImage const& mask) {
            auto* mixed_line = reinterpret_cast<MixedPixel*>(mixedImage.bits());
            const int mixed_stride = mixedImage.bytesPerLine() / sizeof(MixedPixel);
            const uint32_t* foreground_line = foreground.data();
            const int foreground_stride = foreground.wordsPerLine();
            uint32_t const* mask_line = mask.data();
            int const mask_stride = mask.wordsPerLine();
            const int width = mixedImage.width();
            const int height = mixedImage.height();
            const uint32_t msb = uint32_t(1) << 31;

            for (int y = 0; y < height; ++y) {
                for (int x = 0; x < width; ++x) {
                    if (mask_line[x >> 5] & (msb >> (x & 31))) {
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
                mask_line += mask_stride;
            }
        }

        void combineImageMono(QImage& mixedImage,
                              BinaryImage const& foreground,
                              BinaryImage const& mask) {
            if ((mixedImage.format() != QImage::Format_Indexed8)
                && (mixedImage.format() != QImage::Format_RGB32)
                && (mixedImage.format() != QImage::Format_ARGB32)) {
                throw std::invalid_argument("Error: wrong image format.");
            }

            if (mixedImage.format() == QImage::Format_Indexed8) {
                combineImageMono<uint8_t>(mixedImage, foreground, mask);
            } else {
                combineImageMono<uint32_t>(mixedImage, foreground, mask);
            }
        }

        template<typename MixedPixel>
        void combineImageColor(QImage& mixedImage,
                               QImage const& foreground) {
            auto* mixed_line = reinterpret_cast<MixedPixel*>(mixedImage.bits());
            const int mixed_stride = mixedImage.bytesPerLine() / sizeof(MixedPixel);
            const auto* foreground_line = reinterpret_cast<const MixedPixel*>(foreground.bits());
            const int foreground_stride = foreground.bytesPerLine() / sizeof(MixedPixel);
            const int width = mixedImage.width();
            const int height = mixedImage.height();
            const auto msb = uint32_t(0x00ffffff);

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

        void combineImageColor(QImage& mixedImage,
                               QImage const& foreground) {
            if ((mixedImage.format() != QImage::Format_Indexed8)
                && (mixedImage.format() != QImage::Format_RGB32)
                && (mixedImage.format() != QImage::Format_ARGB32)) {
                throw std::invalid_argument("Error: wrong image format.");
            }

            if (mixedImage.format() == QImage::Format_Indexed8) {
                combineImageColor<uint8_t>(mixedImage, foreground);
            } else {
                combineImageColor<uint32_t>(mixedImage, foreground);
            }
        }

        template<typename MixedPixel>
        void combineImageColor(QImage& mixedImage,
                               QImage const& foreground,
                               BinaryImage const& mask) {
            auto* mixed_line = reinterpret_cast<MixedPixel*>(mixedImage.bits());
            const int mixed_stride = mixedImage.bytesPerLine() / sizeof(MixedPixel);
            const auto* foreground_line = reinterpret_cast<const MixedPixel*>(foreground.bits());
            const int foreground_stride = foreground.bytesPerLine() / sizeof(MixedPixel);
            uint32_t const* mask_line = mask.data();
            int const mask_stride = mask.wordsPerLine();
            const int width = mixedImage.width();
            const int height = mixedImage.height();
            const uint32_t msb = uint32_t(1) << 31;

            for (int y = 0; y < height; ++y) {
                for (int x = 0; x < width; ++x) {
                    if (mask_line[x >> 5] & (msb >> (x & 31))) {
                        mixed_line[x] = foreground_line[x];
                    }
                }
                mixed_line += mixed_stride;
                foreground_line += foreground_stride;
                mask_line += mask_stride;
            }
        }

        void combineImageColor(QImage& mixedImage,
                               QImage const& foreground,
                               BinaryImage const& mask) {
            if ((mixedImage.format() != QImage::Format_Indexed8)
                && (mixedImage.format() != QImage::Format_RGB32)
                && (mixedImage.format() != QImage::Format_ARGB32)) {
                throw std::invalid_argument("Error: wrong image format.");
            }

            if (mixedImage.format() == QImage::Format_Indexed8) {
                combineImageColor<uint8_t>(mixedImage, foreground, mask);
            } else {
                combineImageColor<uint32_t>(mixedImage, foreground, mask);
            }
        }

        void combineImage(QImage& mixedImage,
                          QImage const& foreground) {
            if ((mixedImage.format() != QImage::Format_Indexed8)
                && (mixedImage.format() != QImage::Format_RGB32)
                && (mixedImage.format() != QImage::Format_ARGB32)) {
                throw std::invalid_argument("Error: wrong image format.");
            }

            if ((foreground.format() == QImage::Format_Mono)
                || (foreground.format() == QImage::Format_MonoLSB)) {
                combineImageMono(mixedImage, BinaryImage(foreground));
            } else {
                combineImageColor(mixedImage, foreground);
            }
        }

        void combineImage(QImage& mixedImage,
                          QImage const& foreground,
                          BinaryImage const& mask) {
            if ((mixedImage.format() != QImage::Format_Indexed8)
                && (mixedImage.format() != QImage::Format_RGB32)
                && (mixedImage.format() != QImage::Format_ARGB32)) {
                throw std::invalid_argument("Error: wrong image format.");
            }

            if ((foreground.format() == QImage::Format_Mono)
                || (foreground.format() == QImage::Format_MonoLSB)) {
                combineImageMono(mixedImage, BinaryImage(foreground), mask);
            } else {
                combineImageColor(mixedImage, foreground, mask);
            }
        }

        template<typename MixedPixel>
        void applyMask(QImage& image, const BinaryImage& bw_mask) {
            auto* image_line = reinterpret_cast<MixedPixel*>(image.bits());
            const int image_stride = image.bytesPerLine() / sizeof(MixedPixel);
            const uint32_t* bw_mask_line = bw_mask.data();
            const int bw_mask_stride = bw_mask.wordsPerLine();
            const int width = image.width();
            const int height = image.height();
            const uint32_t msb = uint32_t(1) << 31;
            const auto fillingPixel = static_cast<MixedPixel>(0xffffffff);

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

        void applyMask(QImage& image, const BinaryImage& bw_mask) {
            if ((image.format() != QImage::Format_Indexed8)
                && (image.format() != QImage::Format_RGB32)
                && (image.format() != QImage::Format_ARGB32)) {
                throw std::invalid_argument("Error: wrong image format.");
            }

            if (image.format() == QImage::Format_Indexed8) {
                applyMask<uint8_t>(image, bw_mask);
            } else {
                applyMask<uint32_t>(image, bw_mask);
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

    SplitImage::SplitImage(const QImage& foreground, const QImage& background, const QImage& originalBackground)
            : SplitImage(foreground, background) {
        if (isNull()) {
            return;
        }

        if (originalBackground.size() != background.size()) {
            return;
        }

        if (originalBackground.format() != background.format()) {
            return;
        }

        if ((originalBackground.format() != QImage::Format_Indexed8)
            && (originalBackground.format() != QImage::Format_RGB32)
            && (originalBackground.format() != QImage::Format_ARGB32)) {
            return;
        }

        if (!((foreground.format() == QImage::Format_Mono)
              || (foreground.format() == QImage::Format_MonoLSB))) {
            return;
        }

        originalBackgroundImage = originalBackground;
    }

    QImage SplitImage::toImage() const {
        if (isNull()) {
            return QImage();
        }

        if (originalBackgroundImage.isNull()) {
            if (!mask.isNull()) {
                return backgroundImage;
            }

            QImage dst(backgroundImage);
            combineImage(dst, foregroundImage);

            return dst;
        } else {
            QImage dst(originalBackgroundImage);

            {
                BinaryImage backgroundMask = BinaryImage(originalBackgroundImage, BinaryThreshold(1));
                combineImage(dst, backgroundImage, backgroundMask);
            }
            {
                BinaryImage foregroundMask = BinaryImage(originalBackgroundImage, BinaryThreshold(255)).inverted();
                combineImage(dst, (mask.isNull()) ? foregroundImage : backgroundImage, foregroundMask);
            }

            return dst;
        }
    }

    QImage SplitImage::getForegroundImage() const {
        if (!mask.isNull()) {
            QImage foreground(backgroundImage);
            applyMask(foreground, mask);
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
            applyMask(background, mask.inverted());

            return background;
        }

        return backgroundImage;
    }

    void SplitImage::setBackgroundImage(const QImage& backgroundImage) {
        SplitImage::backgroundImage = backgroundImage;
    }

    void SplitImage::applyToLayerImages(const std::function<void(QImage&)>& consumer) {
        if (!foregroundImage.isNull()) {
            consumer(foregroundImage);
        }
        if (!backgroundImage.isNull()) {
            consumer(backgroundImage);
        }
        if (!originalBackgroundImage.isNull()) {
            consumer(originalBackgroundImage);
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

    const QImage& SplitImage::getOriginalBackgroundImage() const {
        return originalBackgroundImage;
    }

    void SplitImage::setOriginalBackgroundImage(const QImage& originalBackgroundImage) {
        SplitImage::originalBackgroundImage = originalBackgroundImage;
    }
} // namespace output
