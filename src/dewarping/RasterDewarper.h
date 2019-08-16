// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef DEWARPING_RASTER_DEWARPER_H_
#define DEWARPING_RASTER_DEWARPER_H_

class QImage;
class QSize;
class QRectF;
class QColor;

namespace dewarping {
class CylindricalSurfaceDewarper;

class RasterDewarper {
 public:
  static QImage dewarp(const QImage& src,
                       const QSize& dst_size,
                       const CylindricalSurfaceDewarper& distortion_model,
                       const QRectF& model_domain,
                       const QColor& background_color);
};
}  // namespace dewarping
#endif
