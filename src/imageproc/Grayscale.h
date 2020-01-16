// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_IMAGEPROC_GRAYSCALE_H_
#define SCANTAILOR_IMAGEPROC_GRAYSCALE_H_

#include <QColor>
#include <QVector>

#include "GrayImage.h"

class QImage;
class QSize;

namespace imageproc {
class BinaryImage;

class GrayscaleHistogram {
 public:
  explicit GrayscaleHistogram(const QImage& img);

  GrayscaleHistogram(const QImage& img, const BinaryImage& mask);

  inline int& operator[](int idx) { return m_pixels[idx]; }

  inline int operator[](int idx) const { return m_pixels[idx]; }

 private:
  void fromMonoImage(const QImage& img);

  void fromMonoMSBImage(const QImage& img, const BinaryImage& mask);

  void fromGrayscaleImage(const QImage& img);

  void fromGrayscaleImage(const QImage& img, const BinaryImage& mask);

  void fromAnyImage(const QImage& img);

  void fromAnyImage(const QImage& img, const BinaryImage& mask);

  int m_pixels[256] = {0};
};


/**
 * \brief Create a 256-element grayscale palette.
 */
QVector<QRgb> createGrayscalePalette();

/**
 * \brief Convert an image from any format to grayscale.
 *
 * \param src The source image in any format.
 * \return A grayscale image with proper palette.  Null will be returned
 *         if \p src was null.
 */
QImage toGrayscale(const QImage& src);

/**
 * \brief Stretch the distribution of gray levels to cover the whole range.
 *
 * \param src The source image.  It doesn't have to be grayscale.
 * \param blackClipFraction The fraction of pixels (fractions of 1) that are
 *        allowed to go negative.  Such pixels will be clipped to 0 (black).
 * \param whiteClipFraction The fraction of pixels (fractions of 1) that are
 *        allowed to exceed the maximum brightness level.  Such pixels will be
 *        clipped to 255 (white).
 * \return A grayscale image, or a null image, if \p src was null.
 */
GrayImage stretchGrayRange(const GrayImage& src, double blackClipFraction = 0.0, double whiteClipFraction = 0.0);

/**
 * \brief Create a grayscale image consisting of a 1 pixel frame and an inner area.
 *
 * \param size The size of the image including the frame.
 * \param innerColor The gray level of the inner area.  Defaults to white.
 * \param frameColor The gray level of the frame area.  Defaults to black.
 * \return The resulting image.
 */
GrayImage createFramedImage(const QSize& size, unsigned char innerColor = 0xff, unsigned char frameColor = 0x00);

/**
 * \brief Find the darkest gray level of an image.
 *
 * \param image The image to process.  If it's null, 0xff will
 *        be returned as the darkest image.  If it's not grayscale,
 *        a grayscale copy will be created.
 */
unsigned char darkestGrayLevel(const QImage& image);
}  // namespace imageproc
#endif  // ifndef SCANTAILOR_IMAGEPROC_GRAYSCALE_H_
