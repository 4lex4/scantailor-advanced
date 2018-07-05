/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C)  Joseph Artsimovich <joseph.artsimovich@gmail.com>

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

#include "PhysSizeCalc.h"
#include <UnitsConverter.h>
#include "ImageTransformation.h"

namespace select_content {
PhysSizeCalc::PhysSizeCalc() = default;

PhysSizeCalc::PhysSizeCalc(const ImageTransformation& xform)
    : m_virtToPhys(xform.transformBack() * UnitsConverter(xform.origDpi()).transform(PIXELS, MILLIMETRES)) {}

QSizeF PhysSizeCalc::sizeMM(const QRectF& rect_px) const {
  const QPolygonF poly_mm(m_virtToPhys.map(rect_px));
  const QSizeF size_mm(QLineF(poly_mm[0], poly_mm[1]).length(), QLineF(poly_mm[1], poly_mm[2]).length());

  return size_mm;
}
}  // namespace select_content
