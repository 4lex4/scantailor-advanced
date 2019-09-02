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

#ifndef IMAGEPROC_MORPHOLOGY_H_
#define IMAGEPROC_MORPHOLOGY_H_

#include <vector>
#include "BWColor.h"

class QSize;
class QRect;
class QPoint;

namespace imageproc {
class BinaryImage;
class GrayImage;

class Brick {
 public:
  /**
   * \brief Constructs a brick with origin at the center.
   */
  Brick(const QSize& size);

  /**
   * \brief Constructs a brick with origin specified relative to its size.
   *
   * For example, a 3x3 brick with origin at the center would be
   * constructed as follows:
   * \code
   * Brick brick(QSize(3, 3), QPoint(1, 1));
   * \endcode
   * \note Origin doesn't have to be inside the brick.
   */
  Brick(const QSize& size, const QPoint& origin);

  /**
   * \brief Constructs a brick by specifying its bounds.
   *
   * Note that all bounds are inclusive.  The order of the arguments
   * is the same as for QRect::adjust().
   */
  Brick(int min_x, int min_y, int max_x, int max_y);

  /**
   * \brief Get the minimum (inclusive) X offset from the origin.
   */
  int minX() const { return m_minX; }

  /**
   * \brief Get the maximum (inclusive) X offset from the origin.
   */
  int maxX() const { return m_maxX; }

  /**
   * \brief Get the minimum (inclusive) Y offset from the origin.
   */
  int minY() const { return m_minY; }

  /**
   * \brief Get the maximum (inclusive) Y offset from the origin.
   */
  int maxY() const { return m_maxY; }

  int width() const { return m_maxX - m_minX + 1; }

  int height() const { return m_maxY - m_minY + 1; }

  bool isEmpty() const { return m_minX > m_maxX || m_minY > m_maxY; }

  /**
   * \brief Flips the brick both horizontally and vertically around the origin.
   */
  void flip();

  /**
   * \brief Returns a brick flipped both horizontally and vertically around the origin.
   */
  Brick flipped() const;

 private:
  int m_minX;
  int m_maxX;
  int m_minY;
  int m_maxY;
};


/**
 * \brief Turn every black pixel into a brick of black pixels.
 *
 * \param src The source image.
 * \param brick The brick to turn each black pixel into.
 * \param dst_area The area in source image coordinates that
 *        will be returned as a destination image. It doesn't have
 *        to fit into the source image area.
 * \param src_surroundings The color of pixels that are assumed to
 *        surround the source image.
 */
BinaryImage dilateBrick(const BinaryImage& src,
                        const Brick& brick,
                        const QRect& dst_area,
                        BWColor src_surroundings = WHITE);

/**
 * \brief Same as above, but assumes dst_rect == src.rect()
 */
BinaryImage dilateBrick(const BinaryImage& src, const Brick& brick, BWColor src_surroundings = WHITE);

/**
 * \brief Spreads darker pixels over the brick's area.
 *
 * \param src The source image.
 * \param brick The area to spread darker pixels into.
 * \param dst_area The area in source image coordinates that
 *        will be returned as a destination image. It doesn't have
 *        to fit into the source image area.
 * \param src_surroundings The color of pixels that are assumed to
 *        surround the source image.
 */
GrayImage dilateGray(const GrayImage& src,
                     const Brick& brick,
                     const QRect& dst_area,
                     unsigned char src_surroundings = 0xff);

/**
 * \brief Same as above, but assumes dst_rect == src.rect()
 */
GrayImage dilateGray(const GrayImage& src, const Brick& brick, unsigned char src_surroundings = 0xff);

/**
 * \brief Turn every white pixel into a brick of white pixels.
 *
 * \param src The source image.
 * \param brick The brick to turn each white pixel into.
 * \param dst_area The area in source image coordinates that
 *        will be returned as a destination image. It doesn't have
 *        to fit into the source image area.
 * \param src_surroundings The color of pixels that are assumed to
 *        surround the source image.
 */
BinaryImage erodeBrick(const BinaryImage& src,
                       const Brick& brick,
                       const QRect& dst_area,
                       BWColor src_surroundings = BLACK);

/**
 * \brief Same as above, but assumes dst_rect == src.rect()
 */
BinaryImage erodeBrick(const BinaryImage& src, const Brick& brick, BWColor src_surroundings = BLACK);

/**
 * \brief Spreads lighter pixels over the brick's area.
 *
 * \param src The source image.
 * \param brick The area to spread lighter pixels into.
 * \param dst_area The area in source image coordinates that
 *        will be returned as a destination image. It doesn't have
 *        to fit into the source image area.
 * \param src_surroundings The color of pixels that are assumed to
 *        surround the source image.
 */
GrayImage erodeGray(const GrayImage& src,
                    const Brick& brick,
                    const QRect& dst_area,
                    unsigned char src_surroundings = 0x00);

/**
 * \brief Same as above, but assumes dst_rect == src.rect()
 */
GrayImage erodeGray(const GrayImage& src, const Brick& brick, unsigned char src_surroundings = 0x00);

/**
 * \brief Turn the black areas where the brick doesn't fit, into white.
 *
 * \param src The source image.
 * \param brick The brick to fit into black areas.
 * \param dst_area The area in source image coordinates that
 *        will be returned as a destination image. It doesn't have
 *        to fit into the source image area.
 * \param src_surroundings The color of pixels that are assumed to
 *        surround the source image.  If set to BLACK, a brick will be able
 *        to fit by going partially off-screen (off the source
 *        image area actually).
 */
BinaryImage openBrick(const BinaryImage& src,
                      const QSize& brick,
                      const QRect& dst_area,
                      BWColor src_surroundings = WHITE);

/**
 * \brief Same as above, but assumes dst_rect == src.rect()
 */
BinaryImage openBrick(const BinaryImage& src, const QSize& brick, BWColor src_surroundings = WHITE);

/**
 * \brief Remove dark areas smaller than the structuring element.
 *
 * \param src The source image.
 * \param brick The structuring element.
 * \param dst_area The area in source image coordinates that
 *        will be returned as a destination image. It doesn't have
 *        to fit into the source image area.
 * \param src_surroundings The color of pixels that are assumed to
 *        surround the source image.
 */
GrayImage openGray(const GrayImage& src, const QSize& brick, const QRect& dst_area, unsigned char src_surroundings);

/**
 * \brief Same as above, but assumes dst_rect == src.rect()
 */
GrayImage openGray(const GrayImage& src, const QSize& brick, unsigned char src_surroundings);

/**
 * \brief Turn the white areas where the brick doesn't fit, into black.
 *
 * \param src The source image.
 * \param brick The brick to fit into white areas.
 * \param dst_area The area in source image coordinates that
 *        will be returned as a destination image. It doesn't have
 *        to fit into the source image area.
 * \param src_surroundings The color of pixels that are assumed to
 *        surround the source image.  If set to WHITE, a brick will be able
 *        to fit by going partially off-screen (off the source
 *        image area actually).
 */
BinaryImage closeBrick(const BinaryImage& src,
                       const QSize& brick,
                       const QRect& dst_area,
                       BWColor src_surroundings = WHITE);

/**
 * \brief Same as above, but assumes dst_rect == src.rect()
 */
BinaryImage closeBrick(const BinaryImage& src, const QSize& brick, BWColor src_surroundings = WHITE);

/**
 * \brief Remove light areas smaller than the structuring element.
 *
 * \param src The source image.
 * \param brick The structuring element.
 * \param dst_area The area in source image coordinates that
 *        will be returned as a destination image. It doesn't have
 *        to fit into the source image area.
 * \param src_surroundings The color of pixels that are assumed to
 *        surround the source image.
 */
GrayImage closeGray(const GrayImage& src, const QSize& brick, const QRect& dst_area, unsigned char src_surroundings);

/**
 * \brief Same as above, but assumes dst_rect == src.rect()
 */
GrayImage closeGray(const GrayImage& src, const QSize& brick, unsigned char src_surroundings);

/**
 * \brief Extracts small elements and details from given image,
 *        i.e. places where the structuring element does not fit in,
 *        and are brighter than their surroundings.
 *
 * \param src The source image.
 * \param brick The brick to fit into black areas.
 * \param dst_area The area in source image coordinates that
 *        will be returned as a destination image. It have
 *        to fit into the source image area.
 * \param src_surroundings The color of pixels that are assumed to
 *        surround the source image.
 */
BinaryImage whiteTopHatTransform(const BinaryImage& src,
                                 const QSize& brick,
                                 const QRect& dst_area,
                                 BWColor src_surroundings = WHITE);

/**
 * \brief Same as above, but assumes dst_rect == src.rect()
 */
BinaryImage whiteTopHatTransform(const BinaryImage& src, const QSize& brick, BWColor src_surroundings = WHITE);

/**
 * \brief Extracts small elements and details from given image,
 *        i.e. places where the structuring element does not fit in,
 *        and are darker than their surroundings.
 *
 * \param src The source image.
 * \param brick The brick to fit into white areas.
 * \param dst_area The area in source image coordinates that
 *        will be returned as a destination image. It have
 *        to fit into the source image area.
 * \param src_surroundings The color of pixels that are assumed to
 *        surround the source image.
 */
BinaryImage blackTopHatTransform(const BinaryImage& src,
                                 const QSize& brick,
                                 const QRect& dst_area,
                                 BWColor src_surroundings = WHITE);

/**
 * \brief Same as above, but assumes dst_rect == src.rect()
 */
BinaryImage blackTopHatTransform(const BinaryImage& src, const QSize& brick, BWColor src_surroundings = WHITE);

/**
 * \brief Performs a hit-miss matching operation.
 *
 * \param src The input image.
 * \param src_surroundings The color that is assumed to be outside of the
 *        input image.
 * \param hits Offsets to hit positions relative to the origin point.
 * \param misses Offsets to miss positions relative to the origin point.
 * \return A binary image where black pixels indicate a successful pattern match.
 */
BinaryImage hitMissMatch(const BinaryImage& src,
                         BWColor src_surroundings,
                         const std::vector<QPoint>& hits,
                         const std::vector<QPoint>& misses);

/**
 * \brief A more user-friendly version of a hit-miss match operation.
 *
 * \param src The input image.
 * \param src_surroundings The color that is assumed to be outside of the
 *        input image.
 * \param pattern A string representing a pattern.  Example:
 * \code
 * const char* pattern =
 *  "?X?"
 *  "X X"
 *  "?X?";
 * \endcode
 * Here X stads for a hit (black pixel) and [space] stands for a miss
 * (white pixel).  Question marks indicate pixels that we are not interested in.
 * \param pattern_width The width of the pattern.
 * \param pattern_height The height of the pattern.
 * \param pattern_origin A point usually within the pattern indicating where
 *        to place a mark if the pattern matches.
 * \return A binary image where black pixels indicate a successful pattern match.
 */
BinaryImage hitMissMatch(const BinaryImage& src,
                         BWColor src_surroundings,
                         const char* pattern,
                         int pattern_width,
                         int pattern_height,
                         const QPoint& pattern_origin);

/**
 * \brief Does a hit-miss match and modifies user-specified pixels.
 *
 * \param src The input image.
 * \param src_surroundings The color that is assumed to be outside of the
 *        input image.
 * \param pattern A string representing a pattern.  Example:
 * \code
 * const char* pattern =
 *  " - "
 *  "X+X"
 *  "XXX";
 * \endcode
 * Pattern characters have the following meaning:\n
 * 'X': A black pixel.\n
 * ' ': A white pixel.\n
 * '-': A black pixel we want to turn into white.\n
 * '+': A white pixel we want to turn into black.\n
 * '?': Any pixel, we don't care which.\n
 * \param pattern_width The width of the pattern.
 * \param pattern_height The height of the pattern.
 * \return The result of a match-and-replace operation.
 */
BinaryImage hitMissReplace(const BinaryImage& src,
                           BWColor src_surroundings,
                           const char* pattern,
                           int pattern_width,
                           int pattern_height);

/**
 * \brief Does a hit-miss match and modifies user-specified pixels.
 *
 * \param[in,out] img The image to make replacements in.
 * \param src_surroundings The color that is assumed to be outside of the
 *        input image.
 * \param pattern A string representing a pattern.  Example:
 * \code
 * const char* pattern =
 *  " - "
 *  "X+X"
 *  "XXX";
 * \endcode
 * Pattern characters have the following meaning:\n
 * 'X': A black pixel.\n
 * ' ': A white pixel.\n
 * '-': A black pixel we want to turn into white.\n
 * '+': A white pixel we want to turn into black.\n
 * '?': Any pixel, we don't care which.\n
 * \param pattern_width The width of the pattern.
 * \param pattern_height The height of the pattern.
 */
void hitMissReplaceInPlace(BinaryImage& img,
                           BWColor src_surroundings,
                           const char* pattern,
                           int pattern_width,
                           int pattern_height);
}  // namespace imageproc
#endif  // ifndef IMAGEPROC_MORPHOLOGY_H_
