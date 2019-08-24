// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "PhysSizeCalc.h"
#include <UnitsConverter.h>
#include "ImageTransformation.h"

namespace select_content {
PhysSizeCalc::PhysSizeCalc() = default;

PhysSizeCalc::PhysSizeCalc(const ImageTransformation& xform)
    : m_virtToPhys(xform.transformBack() * UnitsConverter(xform.origDpi()).transform(PIXELS, MILLIMETRES)) {}

QSizeF PhysSizeCalc::sizeMM(const QRectF& rectPx) const {
  const QPolygonF polyMm(m_virtToPhys.map(rectPx));
  const QSizeF sizeMm(QLineF(polyMm[0], polyMm[1]).length(), QLineF(polyMm[1], polyMm[2]).length());

  return sizeMm;
}
}  // namespace select_content
