
#include "BackgroundColorCalculator.h"
#include <cassert>
#include "Binarize.h"
#include "DebugImages.h"
#include "Grayscale.h"
#include "Morphology.h"
#include "PolygonRasterizer.h"
#include "RasterOp.h"


namespace imageproc {
namespace {
class RgbHistogram {
 public:
  explicit RgbHistogram(const QImage& img);

  RgbHistogram(const QImage& img, const BinaryImage& mask);

  const int* redChannel() const { return m_red; }

  const int* greenChannel() const { return m_green; }

  const int* blueChannel() const { return m_blue; }

 private:
  void fromRgbImage(const QImage& img);

  void fromRgbImage(const QImage& img, const BinaryImage& mask);

  int m_red[256];
  int m_green[256];
  int m_blue[256];
};

RgbHistogram::RgbHistogram(const QImage& img) {
  memset(m_red, 0, sizeof(m_red));
  memset(m_green, 0, sizeof(m_green));
  memset(m_blue, 0, sizeof(m_blue));

  if (img.isNull()) {
    return;
  }

  if (!((img.format() == QImage::Format_RGB32) || (img.format() == QImage::Format_ARGB32))) {
    throw std::invalid_argument("RgbHistogram: wrong image format");
  }

  fromRgbImage(img);
}

RgbHistogram::RgbHistogram(const QImage& img, const BinaryImage& mask) {
  memset(m_red, 0, sizeof(m_red));
  memset(m_green, 0, sizeof(m_green));
  memset(m_blue, 0, sizeof(m_blue));

  if (img.isNull()) {
    return;
  }

  if (!((img.format() == QImage::Format_RGB32) || (img.format() == QImage::Format_ARGB32))) {
    throw std::invalid_argument("RgbHistogram: wrong image format");
  }

  if (img.size() != mask.size()) {
    throw std::invalid_argument("RgbHistogram: img and mask have different sizes");
  }

  fromRgbImage(img, mask);
}

void RgbHistogram::fromRgbImage(const QImage& img) {
  const auto* img_line = reinterpret_cast<const uint32_t*>(img.bits());
  const int img_stride = img.bytesPerLine() / sizeof(uint32_t);

  const int width = img.width();
  const int height = img.height();

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      ++m_red[(img_line[x] >> 16) & 0xff];
      ++m_green[(img_line[x] >> 8) & 0xff];
      ++m_blue[img_line[x] & 0xff];
    }
    img_line += img_stride;
  }
}

void RgbHistogram::fromRgbImage(const QImage& img, const BinaryImage& mask) {
  const auto* img_line = reinterpret_cast<const uint32_t*>(img.bits());
  const int img_stride = img.bytesPerLine() / sizeof(uint32_t);
  const uint32_t* mask_line = mask.data();
  const int mask_stride = mask.wordsPerLine();

  const int width = img.width();
  const int height = img.height();
  const uint32_t msb = uint32_t(1) << 31;

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

void checkImageIsValid(const QImage& img) {
  if (!((img.format() == QImage::Format_RGB32) || (img.format() == QImage::Format_ARGB32)
        || ((img.format() == QImage::Format_Indexed8) && img.isGrayscale()))) {
    throw std::invalid_argument("BackgroundColorCalculator: wrong image format");
  }
  if (img.isNull()) {
    throw std::invalid_argument("BackgroundColorCalculator: image is null.");
  }
}
}  // namespace

uint8_t BackgroundColorCalculator::calcDominantLevel(const int* hist) {
  int integral_hist[256];
  integral_hist[0] = hist[0];
  for (int i = 1; i < 256; ++i) {
    integral_hist[i] = hist[i] + integral_hist[i - 1];
  }

  const int num_colors = 256;
  const int window_size = 10;

  int best_pos = 0;
  int best_sum = integral_hist[window_size - 1];
  for (int i = 1; i <= num_colors - window_size; ++i) {
    const int sum = integral_hist[i + window_size - 1] - integral_hist[i - 1];
    if (sum > best_sum) {
      best_sum = sum;
      best_pos = i;
    }
  }

  int half_sum = 0;
  for (int i = best_pos; i < best_pos + window_size; ++i) {
    half_sum += hist[i];
    if (half_sum >= best_sum / 2) {
      return static_cast<uint8_t>(i);
    }
  }

  assert(!"Unreachable");

  return 0;
}  // BackgroundColorCalculator::calcDominantLevel

QColor BackgroundColorCalculator::calcDominantBackgroundColor(const QImage& img) const {
  checkImageIsValid(img);

  BinaryImage background_mask(img, BinaryThreshold::otsuThreshold(img));
  if (isBlackOnWhite(background_mask)) {
    background_mask.invert();
  }

  return calcDominantColor(img, background_mask);
}

QColor BackgroundColorCalculator::calcDominantBackgroundColor(const QImage& img,
                                                              const BinaryImage& mask,
                                                              DebugImages* dbg) const {
  checkImageIsValid(img);

  if (img.size() != mask.size()) {
    throw std::invalid_argument("BackgroundColorCalculator: img and mask have different sizes");
  }

  BinaryImage background_mask(img, BinaryThreshold::otsuThreshold(GrayscaleHistogram(img, mask)));
  if (isBlackOnWhite(background_mask, mask)) {
    background_mask.invert();
  }
  rasterOp<RopAnd<RopSrc, RopDst>>(background_mask, mask);
  if (dbg) {
    dbg->add(background_mask, "background_mask");
  }

  return calcDominantColor(img, background_mask);
}

QColor BackgroundColorCalculator::calcDominantBackgroundColor(const QImage& img,
                                                              const QPolygonF& crop_area,
                                                              DebugImages* dbg) const {
  checkImageIsValid(img);

  if (crop_area.intersected(QRectF(img.rect())).isEmpty()) {
    throw std::invalid_argument("BackgroundColorCalculator: the cropping area is wrong.");
  }

  BinaryImage mask(img.size(), BLACK);
  PolygonRasterizer::fillExcept(mask, WHITE, crop_area, Qt::WindingFill);

  return calcDominantBackgroundColor(img, mask, dbg);
}

QColor BackgroundColorCalculator::calcDominantColor(const QImage& img, const BinaryImage& background_mask) {
  if ((img.format() == QImage::Format_Indexed8) && img.isGrayscale()) {
    const GrayscaleHistogram hist(img, background_mask);
    int raw_hist[256];
    grayHistToArray(raw_hist, hist);
    uint8_t dominant_gray = calcDominantLevel(raw_hist);

    return QColor(dominant_gray, dominant_gray, dominant_gray);
  } else {
    const RgbHistogram hist(img, background_mask);
    uint8_t dominant_red = calcDominantLevel(hist.redChannel());
    uint8_t dominant_green = calcDominantLevel(hist.greenChannel());
    uint8_t dominant_blue = calcDominantLevel(hist.blueChannel());

    return QColor(dominant_red, dominant_green, dominant_blue);
  }
}

BackgroundColorCalculator::BackgroundColorCalculator(bool internalBlackOnWhiteDetection)
    : internalBlackOnWhiteDetection(internalBlackOnWhiteDetection) {}

bool BackgroundColorCalculator::isBlackOnWhite(const BinaryImage& img) const {
  if (!internalBlackOnWhiteDetection) {
    return true;
  }
  return (2 * img.countBlackPixels()) <= (img.width() * img.height());
}

bool BackgroundColorCalculator::isBlackOnWhite(BinaryImage img, const BinaryImage& mask) const {
  if (!internalBlackOnWhiteDetection) {
    return true;
  }
  rasterOp<RopAnd<RopSrc, RopDst>>(img, mask);
  return (2 * img.countBlackPixels() <= mask.countBlackPixels());
}
}  // namespace imageproc