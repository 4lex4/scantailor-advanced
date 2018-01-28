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
