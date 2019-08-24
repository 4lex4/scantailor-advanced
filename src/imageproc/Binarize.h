// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_IMAGEPROC_BINARIZE_H_
#define SCANTAILOR_IMAGEPROC_BINARIZE_H_

#include <QSize>

class QImage;

namespace imageproc {
class BinaryImage;

/**
 * \brief Image binarization using Otsu's global thresholding method.
 *
 * N. Otsu (1979). "A threshold selection method from gray-level histograms".
 * http://en.wikipedia.org/wiki/Otsu%27s_method
 */
BinaryImage binarizeOtsu(const QImage& src);

/**
 * \brief Image binarization using Mokji's global thresholding method.
 *
 * M. M. Mokji, S. A. R. Abu-Bakar: Adaptive Thresholding Based on
 * Co-occurrence Matrix Edge Information. Asia International Conference on
 * Modelling and Simulation 2007: 444-450
 * http://www.academypublisher.com/jcp/vol02/no08/jcp02084452.pdf
 *
 * \param src The source image.  May be in any format.
 * \param maxEdgeWidth The maximum gradient length to consider.
 * \param minEdgeMagnitude The minimum color difference in a gradient.
 * \return A black and white image.
 */
BinaryImage binarizeMokji(const QImage& src, unsigned maxEdgeWidth = 3, unsigned minEdgeMagnitude = 20);

/**
 * \brief Image binarization using Sauvola's local thresholding method.
 *
 * Sauvola, J. and M. Pietikainen. 2000. "Adaptive document image binarization".
 * http://www.mediateam.oulu.fi/publications/pdf/24.pdf
 */
BinaryImage binarizeSauvola(const QImage& src, QSize windowSize, double k = 0.34);

/**
 * \brief Image binarization using Wolf's local thresholding method.
 *
 * C. Wolf, J.M. Jolion, F. Chassaing. "Text localization, enhancement and
 * binarization in multimedia documents."
 * http://liris.cnrs.fr/christian.wolf/papers/icpr2002v.pdf
 *
 * \param src The image to binarize.
 * \param windowSize The dimensions of a pixel neighborhood to consider.
 * \param lowerBound The minimum possible gray level that can be made white.
 * \param upperBound The maximum possible gray level that can be made black.
 */
BinaryImage binarizeWolf(const QImage& src,
                         QSize windowSize,
                         unsigned char lowerBound = 1,
                         unsigned char upperBound = 254,
                         double k = 0.3);

BinaryImage peakThreshold(const QImage& image);
}  // namespace imageproc
#endif
