/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C)  Joseph Artsimovich <joseph.artsimovich@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef IMAGEPROC_BINARYIMAGE_H_
#define IMAGEPROC_BINARYIMAGE_H_

#include <QColor>
#include <QRect>
#include <QSize>
#include <cstdint>
#include <vector>
#include "BWColor.h"
#include "BinaryThreshold.h"

class QImage;

namespace imageproc {
/**
 * \brief An image consisting of black and white pixels.
 *
 * The reason for having a separate image class instead of just using
 * QImage is convenience and efficiency concerns.  BinaryImage is a
 * sequence of 32bit words with bytes and bits arranged in such a way
 * that
 * \code
 * word << x
 * word >> x
 * \endcode
 * are equivalent to shifting a group of pixels to the left and to the
 * right respectively.\n
 * Additionally, unlinke QImage, BinaryImage doesn't have a palette,
 * so black pixels are always represented as ones and white pixels as zeros.
 */
class BinaryImage {
 public:
  /**
   * \brief Creates a null image.
   */
  BinaryImage();

  /**
   * \brief Creates a new image.  Image data will be uninitialized.
   *
   * To initialize image data, use fill().
   */
  BinaryImage(int width, int height);

  /**
   * \brief Creates a new image.  Image data will be uninitialized.
   *
   * To initialize image data, use fill().
   */
  explicit BinaryImage(QSize size);

  /**
   * \brief Creates a new image filled with specified color.
   */
  BinaryImage(int width, int height, BWColor color);

  /**
   * \brief Creates a new image filled with specified color.
   */
  BinaryImage(QSize size, BWColor color);

  /**
   * \brief Create a copy of another image.  Copy-on-write is used.
   */
  BinaryImage(const BinaryImage& other);

  /**
   * \brief Create a new image by copying the contents of a QImage.
   *
   * Colors in a QImage are converted to gray first, and then
   * compared against the provided threshold.
   */
  explicit BinaryImage(const QImage& image, BinaryThreshold threshold = BinaryThreshold(128));

  /**
   * \brief Create a new image by copying a part of a QImage.
   *
   * \p rect Must be within image.rect().  If \p rect is empty,
   * a null BinaryImage is constructed.
   *
   * Colors in a QImage are converted to gray first, and then
   * compared against the provided threshold.
   */
  explicit BinaryImage(const QImage& image, const QRect& rect, BinaryThreshold threshold = BinaryThreshold(128));

  ~BinaryImage();

  /**
   * \brief Replaces the current image with a copy of another one.
   *
   * Copy-on-write is used.  This means that several images will share
   * their data, until one of them accesses it in a non-const way,
   * which is when a private copy of data is created for that image.
   */
  BinaryImage& operator=(const BinaryImage& other);

  /**
   * \brief Returns true if the image is null.
   *
   * Null images have zero width, height and wordsPerLine.
   */
  bool isNull() const { return !m_pData; }

  /**
   * \brief Swaps two images.
   *
   * This operations doesn't copy data, it just swaps pointers to it.
   */
  void swap(BinaryImage& other);

  /**
   * \brief Release the image data and return it as a new image.
   *
   * This object becomes null and its data is returned as a new image.
   */
  BinaryImage release();

  /**
   * \brief Invert black and white colors.
   */
  void invert();

  /**
   * \brief Creates an inverted version of this image.
   */
  BinaryImage inverted() const;

  /**
   * \brief Fills the whole image with either white or black color.
   */
  void fill(BWColor color);

  /**
   * \brief Fills a portion of the image with either white or black color.
   *
   * If the bounding rectangle exceedes the image area, it's automatically truncated.
   */
  void fill(const QRect& rect, BWColor color);

  /**
   * \brief Fills a portion of the image with either white or black color.
   *
   * If the bounding rectangle exceedes the image area, it's automatically truncated.
   */
  void fillExcept(const QRect& rect, BWColor color);

  /**
   * \brief Fills the area inside outer_rect but not inside inner_rect.
   *
   * If inner or outer rectangles exceed the image area, or if inner rectangle
   * exceedes the outer rectangle area, they will be automatically truncated.
   */
  void fillFrame(const QRect& outer_rect, const QRect& inner_rect, BWColor color);

  int countBlackPixels() const;

  int countWhitePixels() const;

  /**
   * \brief Return the number of black pixels in a specified area.
   *
   * The specified rectangle is allowed to extend beyond the image area.
   * In this case, pixels that are outside of the image won't be counted.
   */
  int countBlackPixels(const QRect& rect) const;

  /**
   * \brief Return the number of white pixels in a specified area.
   *
   * The specified rectangle is allowed to extend beyond the image area.
   * In this case, pixels that are outside of the image won't be counted.
   */
  int countWhitePixels(const QRect& rect) const;

  /**
   * \brief Calculates the bounding box of either black or white content.
   */
  QRect contentBoundingBox(BWColor content_color = BLACK) const;

  void rectangularizeAreas(std::vector<QRect>& areas, BWColor content_color, int sensitivity);

  int width() const { return m_width; }

  int height() const { return m_height; }

  QRect rect() const { return QRect(0, 0, m_width, m_height); }

  QSize size() const { return QSize(m_width, m_height); }

  /**
   * \brief Returns the number of 32bit words per line.
   *
   * This value is usually (width + 31) / 32, but it can also
   * be bigger than that.
   */
  int wordsPerLine() const { return m_wpl; }

  /**
   * \brief Returns a pointer to non-const image data.
   * \return Image data, or 0 in case of a null image.
   *
   * This may trigger copy-on-write.  The pointer returned is only
   * valid until you create a copy of this image.  After that, both
   * images will share the same data, and you will need to call
   * data() again if you want to continue writing to this image.
   */
  uint32_t* data();

  /**
   * \brief Returns a pointer to const image data.
   * \return Image data, or 0 in case of a null image.
   *
   * The pointer returned is only valid until call a non-const
   * version of data(), because that may trigger copy-on-write.
   */
  const uint32_t* data() const;

  /**
   * \brief Convert to a QImage with Format_Mono.
   */
  QImage toQImage() const;

  /**
   * \brief Convert to an ARGB32_Premultiplied image, where white pixels become transparent.
   *
   * Opaque (black) pixels take the specified color.  Colors with alpha channel are supported.
   */
  QImage toAlphaMask(const QColor& color) const;

  void setPixel(int x, int y, BWColor color);

  BWColor getPixel(int x, int y);

 private:
  class SharedData;

  BinaryImage(int width, int height, SharedData* data);

  void copyIfShared();

  void fillRectImpl(uint32_t* data, const QRect& rect, BWColor color);

  static BinaryImage fromMono(const QImage& image);

  static BinaryImage fromMono(const QImage& image, const QRect& rect);

  static BinaryImage fromMonoLSB(const QImage& image);

  static BinaryImage fromMonoLSB(const QImage& image, const QRect& rect);

  static BinaryImage fromIndexed8(const QImage& image, const QRect& rect, int threshold);

  static BinaryImage fromRgb32(const QImage& image, const QRect& rect, int threshold);

  static BinaryImage fromArgb32Premultiplied(const QImage& image, const QRect& rect, int threshold);

  static BinaryImage fromRgb16(const QImage& image, const QRect& rect, int threshold);

  static bool isLineMonotone(const uint32_t* line, int last_word_idx, uint32_t last_word_mask, uint32_t modifier);

  static int leftmostBitOffset(const uint32_t* line, int offset_limit, uint32_t modifier);

  static int rightmostBitOffset(const uint32_t* line, int offset_limit, uint32_t modifier);

  SharedData* m_pData;
  int m_width;
  int m_height;
  int m_wpl;  // words per line
};


inline void swap(BinaryImage& o1, BinaryImage& o2) {
  o1.swap(o2);
}

inline BinaryImage BinaryImage::release() {
  BinaryImage new_img;
  new_img.swap(*this);

  return new_img;
}

/**
 * \brief Compares image data.
 */
bool operator==(const BinaryImage& lhs, const BinaryImage& rhs);

/**
 * \brief Compares image data.
 */
inline bool operator!=(const BinaryImage& lhs, const BinaryImage& rhs) {
  return !(lhs == rhs);
}
}  // namespace imageproc
#endif  // ifndef IMAGEPROC_BINARYIMAGE_H_
