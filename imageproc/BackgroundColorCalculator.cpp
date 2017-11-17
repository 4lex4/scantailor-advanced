
#include <assert.h>
#include "BackgroundColorCalculator.h"
#include "Grayscale.h"
#include "Binarize.h"
#include "BinaryImage.h"


namespace imageproc {
    namespace {
        class RgbHistogram {
        public:
            explicit RgbHistogram(QImage const& img);

            RgbHistogram(QImage const& img, BinaryImage const& mask);

            const int* redChannel() const {
                return m_red;
            }

            const int* greenChannel() const {
                return m_green;
            }

            const int* blueChannel() const {
                return m_blue;
            }

        private:
            void fromRgbImage(QImage const& img);

            void fromRgbImage(QImage const& img, BinaryImage const& mask);

            int m_red[256];
            int m_green[256];
            int m_blue[256];
        };

        RgbHistogram::RgbHistogram(QImage const& img) {
            memset(m_red, 0, sizeof(m_red));
            memset(m_green, 0, sizeof(m_green));
            memset(m_blue, 0, sizeof(m_blue));

            if (img.isNull()) {
                return;
            }

            if (!((img.format() == QImage::Format_RGB32)
                  || (img.format() == QImage::Format_ARGB32))) {
                throw std::invalid_argument(
                        "RgbHistogram: wrong image format"
                );
            }

            fromRgbImage(img);
        }

        RgbHistogram::RgbHistogram(QImage const& img, BinaryImage const& mask) {
            memset(m_red, 0, sizeof(m_red));
            memset(m_green, 0, sizeof(m_green));
            memset(m_blue, 0, sizeof(m_blue));

            if (img.isNull()) {
                return;
            }

            if (!((img.format() == QImage::Format_RGB32)
                  || (img.format() == QImage::Format_ARGB32))) {
                throw std::invalid_argument(
                        "RgbHistogram: wrong image format"
                );
            }

            if (img.size() != mask.size()) {
                throw std::invalid_argument(
                        "RgbHistogram: img and mask have different sizes"
                );
            }

            fromRgbImage(img, mask);
        }

        void RgbHistogram::fromRgbImage(QImage const& img) {
            const uint32_t* img_line = reinterpret_cast<const uint32_t*>(img.bits());
            int const img_stride = img.bytesPerLine() / sizeof(uint32_t);

            int const width = img.width();
            int const height = img.height();
            uint32_t const msb = uint32_t(1) << 31;

            for (int y = 0; y < height; ++y) {
                for (int x = 0; x < width; ++x) {
                    ++m_red[(img_line[x] >> 16) & 0xff];
                    ++m_green[(img_line[x] >> 8) & 0xff];
                    ++m_blue[img_line[x] & 0xff];
                }
                img_line += img_stride;
            }
        }

        void RgbHistogram::fromRgbImage(QImage const& img, BinaryImage const& mask) {
            const uint32_t* img_line = reinterpret_cast<const uint32_t*>(img.bits());
            int const img_stride = img.bytesPerLine() / sizeof(uint32_t);
            uint32_t const* mask_line = mask.data();
            int const mask_stride = mask.wordsPerLine();

            int const width = img.width();
            int const height = img.height();
            uint32_t const msb = uint32_t(1) << 31;

            for (int y = 0; y < height; ++y) {
                for (int x = 0; x < width; ++x) {
                    if (mask_line[x >> 5] & (msb >> (x & 31))) {
                        ++m_red[(img_line[x] >> 16) & 0xff];
                        ++m_green[(img_line[x] >> 8) & 0xff];
                        ++m_blue[img_line[x] & 0xff];
                    }
                }
                img_line += img_stride;
                mask_line += mask_stride;
            }
        }

        void grayHistToArray(int* raw_hist, GrayscaleHistogram hist) {
            for (int i = 0; i < 256; ++i) {
                raw_hist[i] = hist[i];
            }
        }
    }

    uint8_t BackgroundColorCalculator::calcDominantLevel(const int* hist) {
        int integral_hist[256];
        integral_hist[0] = hist[0];
        for (int i = 1; i < 256; ++i) {
            integral_hist[i] = hist[i] + integral_hist[i - 1];
        }

        int const num_colors = 256;
        int const window_size = 10;

        int best_pos = 0;
        int best_sum = integral_hist[window_size - 1];
        for (int i = 1; i <= num_colors - window_size; ++i) {
            int const sum = integral_hist[i + window_size - 1] - integral_hist[i - 1];
            if (sum > best_sum) {
                best_sum = sum;
                best_pos = i;
            }
        }

        int half_sum = 0;
        for (int i = best_pos; i < best_pos + window_size; ++i) {
            half_sum += hist[i];
            if (half_sum >= best_sum / 2) {
                return i;
            }
        }

        assert(!"Unreachable");

        return 0;
    }   // BackgroundColorCalculator::calcDominantLevel

    QColor BackgroundColorCalculator::calcDominantBackgroundColor(QImage const& img) {
        if (!((img.format() == QImage::Format_RGB32)
              || (img.format() == QImage::Format_ARGB32)
              || (img.format() == QImage::Format_Indexed8))) {
            throw std::invalid_argument(
                    "BackgroundColorCalculator: wrong image format"
            );
        }

        BinaryImage mask(binarizeOtsu(img));
        if (mask.countBlackPixels() < mask.countWhitePixels()) {
            mask.invert();
        }

        if (img.format() == QImage::Format_Indexed8) {
            GrayscaleHistogram const hist(img, mask);
            int raw_hist[256];
            grayHistToArray(raw_hist, hist);
            uint8_t dominant_gray = calcDominantLevel(raw_hist);

            return QColor(dominant_gray, dominant_gray, dominant_gray);
        } else {
            RgbHistogram const hist(img, mask);
            uint8_t dominant_red = calcDominantLevel(hist.redChannel());
            uint8_t dominant_green = calcDominantLevel(hist.greenChannel());
            uint8_t dominant_blue = calcDominantLevel(hist.blueChannel());

            return QColor(dominant_red, dominant_green, dominant_blue);
        }
    }

    QColor BackgroundColorCalculator::calcDominantBackgroundColorBW(QImage const& img) {
        BinaryImage mask(binarizeOtsu(img));
        if (mask.countBlackPixels() < mask.countWhitePixels()) {
            return Qt::white;
        } else {
            return Qt::black;
        }
    }


}