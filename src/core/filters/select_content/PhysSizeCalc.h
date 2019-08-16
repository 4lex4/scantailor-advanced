// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SELECT_CONTENT_PHYS_SIZE_CALC_H_
#define SELECT_CONTENT_PHYS_SIZE_CALC_H_

#include <QRectF>
#include <QSizeF>
#include <QTransform>

class ImageTransformation;

namespace select_content {
class PhysSizeCalc {
  // Member-wise copying is OK.
 public:
  PhysSizeCalc();

  explicit PhysSizeCalc(const ImageTransformation& xform);

  QSizeF sizeMM(const QRectF& rect_px) const;

 private:
  QTransform m_virtToPhys;
};
}  // namespace select_content
#endif
