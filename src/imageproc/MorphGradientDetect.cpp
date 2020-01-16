// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "MorphGradientDetect.h"

#include "GrayRasterOp.h"
#include "Grayscale.h"
#include "Morphology.h"

namespace imageproc {
GrayImage morphGradientDetectDarkSide(const GrayImage& image, const QSize& area) {
  GrayImage lighter(erodeGray(image, area, 0x00));
  grayRasterOp<GRopUnclippedSubtract<GRopDst, GRopSrc>>(lighter, image);
  return lighter;
}

GrayImage morphGradientDetectLightSide(const GrayImage& image, const QSize& area) {
  GrayImage darker(dilateGray(image, area, 0xff));
  grayRasterOp<GRopUnclippedSubtract<GRopSrc, GRopDst>>(darker, image);
  return darker;
}
}  // namespace imageproc
