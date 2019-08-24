// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_IMAGEPROC_POLYGONRASTERIZER_H_
#define SCANTAILOR_IMAGEPROC_POLYGONRASTERIZER_H_

#include <Qt>
#include "BWColor.h"

class QPolygonF;
class QRectF;
class QImage;

namespace imageproc {
class BinaryImage;

class PolygonRasterizer {
 public:
  static void fill(BinaryImage& image, BWColor color, const QPolygonF& poly, Qt::FillRule fillRule);

  static void fillExcept(BinaryImage& image, BWColor color, const QPolygonF& poly, Qt::FillRule fillRule);

  static void grayFill(QImage& image, unsigned char color, const QPolygonF& poly, Qt::FillRule fillRule);

  static void grayFillExcept(QImage& image, unsigned char color, const QPolygonF& poly, Qt::FillRule fillRule);

 private:
  class Edge;
  class EdgeComponent;
  class EdgeOrderY;
  class EdgeOrderX;

  class Rasterizer;
};
}  // namespace imageproc
#endif  // ifndef SCANTAILOR_IMAGEPROC_POLYGONRASTERIZER_H_
