// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef PAGE_LAYOUT_THUMBNAIL_H_
#define PAGE_LAYOUT_THUMBNAIL_H_

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
  Thumbnail(intrusive_ptr<ThumbnailPixmapCache> thumbnail_cache,
            const QSizeF& max_size,
            const ImageId& image_id,
            const Params& params,
            const ImageTransformation& xform,
            const QPolygonF& phys_content_rect,
            const QRectF& displayArea,
            bool deviant);

  void paintOverImage(QPainter& painter,
                      const QTransform& image_to_display,
                      const QTransform& thumb_to_display) override;

 private:
  Params m_params;
  QRectF m_virtContentRect;
  QRectF m_virtOuterRect;
  bool m_deviant;
};
}  // namespace page_layout
#endif
