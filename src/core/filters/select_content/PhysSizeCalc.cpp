// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

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
