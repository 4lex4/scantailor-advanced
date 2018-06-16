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

#ifndef IMAGEPROC_POLYGONRASTERIZER_H_
#define IMAGEPROC_POLYGONRASTERIZER_H_

#include <Qt>
#include "BWColor.h"

class QPolygonF;
class QRectF;
class QImage;

namespace imageproc {
class BinaryImage;

class PolygonRasterizer {
 public:
  static void fill(BinaryImage& image, BWColor color, const QPolygonF& poly, Qt::FillRule fill_rule);

  static void fillExcept(BinaryImage& image, BWColor color, const QPolygonF& poly, Qt::FillRule fill_rule);

  static void grayFill(QImage& image, unsigned char color, const QPolygonF& poly, Qt::FillRule fill_rule);

  static void grayFillExcept(QImage& image, unsigned char color, const QPolygonF& poly, Qt::FillRule fill_rule);

 private:
  class Edge;
  class EdgeComponent;
  class EdgeOrderY;
  class EdgeOrderX;

  class Rasterizer;
};
}  // namespace imageproc
#endif  // ifndef IMAGEPROC_POLYGONRASTERIZER_H_
