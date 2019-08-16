// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef IMAGEPROC_BAD_ALLOC_IF_NULL_H_
#define IMAGEPROC_BAD_ALLOC_IF_NULL_H_

class QImage;

namespace imageproc {
/**
 * @brief Throw std::bad_alloc exception if the image is null.
 *
 * Qt has this annoying behaviour when an out-of-memory situation
 * in a method of QImage results in either a null image being returned
 * from that method or even "*this" image itself becoming null! Either
 * scenario leads to a crash with high probability. Instead, we prefer
 * to throw std::bad_alloc(), which will result in an "Out of Memory"
 * dialog and a graceful termination.
 *
 * @param image The image to test.
 * @return The image passed as the argument.
 */
const QImage& badAllocIfNull(const QImage& image);
}  // namespace imageproc

#endif
