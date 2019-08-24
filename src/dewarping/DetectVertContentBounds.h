// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_DEWARPING_DETECTVERTCONTENTBOUNDS_H_
#define SCANTAILOR_DEWARPING_DETECTVERTCONTENTBOUNDS_H_

#include <QLineF>
#include <utility>

class DebugImages;

namespace imageproc {
class BinaryImage;
}

namespace dewarping {
/**
 * \brief Detect the left and right content boundaries.
 *
 * \param image The image to work on.
 * \return A pair of left, right boundaries.  These lines will span
 *         from top to bottom of the image, and may be partially or even
 *         completely outside of its bounds.
 *
 * \note This function assumes a clean image, that is no clutter
 *       or speckles, at least not outside of the content area.
 */
std::pair<QLineF, QLineF> detectVertContentBounds(const imageproc::BinaryImage& image, DebugImages* dbg);
}  // namespace dewarping
#endif
