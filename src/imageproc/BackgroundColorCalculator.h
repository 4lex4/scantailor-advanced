// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_IMAGEPROC_BACKGROUNDCOLORCALCULATOR_H_
#define SCANTAILOR_IMAGEPROC_BACKGROUNDCOLORCALCULATOR_H_

#include <cstdint>

class QImage;
class QColor;
class QPolygonF;

namespace imageproc {
class GrayscaleHistogram;
class BinaryImage;

class BackgroundColorCalculator {
 public:
  static QColor calcDominantBackgroundColor(const QImage& img);

  static QColor calcDominantBackgroundColor(const QImage& img, const BinaryImage& mask);

  static QColor calcDominantBackgroundColor(const QImage& img, const QPolygonF& cropArea);

 private:
  static uint8_t calcDominantLevel(const int* hist);

  static QColor calcDominantColor(const QImage& img, const BinaryImage& backgroundMask);
};
}  // namespace imageproc


#endif  // SCANTAILOR_IMAGEPROC_BACKGROUNDCOLORCALCULATOR_H_
