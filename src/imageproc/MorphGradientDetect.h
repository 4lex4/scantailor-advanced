// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef IMAGEPROC_MORPHGRADIENTDETECT_H_
#define IMAGEPROC_MORPHGRADIENTDETECT_H_

class QSize;

namespace imageproc {
class GrayImage;

/**
 * \brief Morphological gradient detection.
 *
 * This function finds the the difference between the gray level of a pixel
 * and the gray level of the lightest pixel in its neighborhood, which
 * is specified by the \p area parameter.
 * The DarkSide in the name suggests that given a dark-to-light transition,
 * the gradient will be detected on the dark side.
 * \par
 * There is no requirement for the source image to be grayscale.  A grayscale
 * version will be created transparently, if necessary.
 * \par
 * Smoothing the image before calling this function is often a good idea,
 * especially for black and white images.
 */
GrayImage morphGradientDetectDarkSide(const GrayImage& image, const QSize& area);

/**
 * \brief Morphological gradient detection.
 *
 * This function finds the the difference between the gray level of a pixel
 * and the gray level of the darkest pixel in its neighborhood, which
 * is specified by the \p area parameter.
 * The LightSide in the name suggests that given a dark-to-light transition,
 * the gradient will be detected on the light side.
 * \par
 * There is no requirement for the source image to be grayscale.  A grayscale
 * version will be created transparently, if necessary.
 * \par
 * Smoothing the image before calling this function is often a good idea,
 * especially for black and white images.
 */
GrayImage morphGradientDetectLightSide(const GrayImage& image, const QSize& area);
}  // namespace imageproc
#endif
