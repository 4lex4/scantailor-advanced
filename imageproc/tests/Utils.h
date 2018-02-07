/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2007-2008  Joseph Artsimovich <joseph_a@mail.ru>

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

#ifndef IMAGEPROC_TESTS_UTILS_H_
#define IMAGEPROC_TESTS_UTILS_H_

class QImage;
class QRect;

namespace imageproc {
    class BinaryImage;

    namespace tests {
        namespace utils {
            BinaryImage randomBinaryImage(int width, int height);

            QImage randomMonoQImage(int width, int height);

            QImage randomGrayImage(int width, int height);

            BinaryImage makeBinaryImage(const int* data, int width, int height);

            QImage makeMonoQImage(const int* data, int width, int height);

            QImage makeGrayImage(const int* data, int width, int height);

            void dumpBinaryImage(const BinaryImage& img, const char* name = nullptr);

            void dumpGrayImage(const QImage& img, const char* name = nullptr);

            bool surroundingsIntact(const QImage& img1, const QImage& img2, const QRect& rect);
        }  // namespace utils
    }  // namespace uests
}  // namespace imageproc
#endif // ifndef IMAGEPROC_TESTS_UTILS_H_
