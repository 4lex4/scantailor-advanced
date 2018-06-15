
#ifndef SCANTAILOR_COLORTABLE_H
#define SCANTAILOR_COLORTABLE_H


#include <QtGui/QImage>
#include <unordered_map>

namespace imageproc {
class ColorTable {
 private:
  QImage image;

 public:
  explicit ColorTable(const QImage& image);

  ColorTable& posterize(int level,
                        bool normalize = false,
                        bool forceBlackAndWhite = false,
                        int normalizeBlackLevel = 0,
                        int normalizeWhiteLevel = 255);

  QVector<QRgb> getPalette() const;

  QImage getImage() const;

  QImage toIndexedImage(const QVector<QRgb>* colorTable = nullptr) const;

 private:
  std::unordered_map<uint32_t, int> paletteFromIndexedWithStatistics() const;

  std::unordered_map<uint32_t, int> paletteFromRgbWithStatistics() const;

  void remapColorsInIndexedImage(const std::unordered_map<uint32_t, uint32_t>& colorMap);

  void remapColorsInRgbImage(const std::unordered_map<uint32_t, uint32_t>& colorMap);

  void buildIndexedImageFromRgb(const std::unordered_map<uint32_t, uint32_t>& colorMap);

  std::unordered_map<uint32_t, uint32_t> normalizePalette(const std::unordered_map<uint32_t, int>& palette,
                                                          int normalizeBlackLevel = 0,
                                                          int normalizeWhiteLevel = 255) const;

  void makeGrayBlackOrWhiteInPlace(QRgb& rgb, const QRgb& normalized) const;
};
}  // namespace imageproc

#endif  // SCANTAILOR_COLORTABLE_H
