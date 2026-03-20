/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2015  Joseph Artsimovich <joseph.artsimovich@gmail.com>

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

#ifndef SCANTAILOR_IMAGEPROC_WIENER_FILTER_H_
#define SCANTAILOR_IMAGEPROC_WIENER_FILTER_H_

#include "GrayImage.h"

class QImage;
class QSize;

namespace imageproc {

class GrayImage;

/**
 * @brief Applies the Wiener filter to a grayscale image.
 *
 * @param image The image to apply the filter to. A null image is allowed.
 * @param window_size The local neighbourhood around a pixel to use.
 * @param noise_sigma The standard deviation of noise in the image.
 * @return The filtered image.
 */
GrayImage wienerFilter(GrayImage const& image, QSize const& window_size, double noise_sigma);

/**
 * @brief An in-place version of wienerFilter().
 * @see wienerFilter()
 */
void wienerFilterInPlace(GrayImage& image, QSize const& window_size, double noise_sigma);


QImage wienerColorFilter(QImage const& image, QSize const& window_size, double coef);

void wienerColorFilterInPlace(QImage& image, QSize const& window_size, double coef);

}  // namespace imageproc

#endif
