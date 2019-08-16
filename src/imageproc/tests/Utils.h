// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

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
}  // namespace tests
}  // namespace imageproc
#endif  // ifndef IMAGEPROC_TESTS_UTILS_H_
