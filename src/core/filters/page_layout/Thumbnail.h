// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_PAGE_LAYOUT_THUMBNAIL_H_
#define SCANTAILOR_PAGE_LAYOUT_THUMBNAIL_H_

#include <QRectF>
#include <QTransform>

#include "ImageTransformation.h"
#include "Params.h"
#include "ThumbnailBase.h"
#include "intrusive_ptr.h"

class ThumbnailPixmapCache;
class ImageId;

namespace page_layout {
class Thumbnail : public ThumbnailBase {
 public:
  Thumbnail(intrusive_ptr<ThumbnailPixmapCache> thumbnailCache,
            const QSizeF& maxSize,
            const ImageId& imageId,
            const Params& params,
            const ImageTransformation& xform,
            const QPolygonF& physContentRect,
            bool deviant);

  void paintOverImage(QPainter& painter, const QTransform& imageToDisplay, const QTransform& thumbToDisplay) override;

 private:
  Params m_params;
  QRectF m_virtContentRect;
  QRectF m_virtOuterRect;
  bool m_deviant;
};
}  // namespace page_layout
#endif
