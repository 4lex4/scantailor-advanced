// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SELECT_CONTENT_THUMBNAIL_H_
#define SELECT_CONTENT_THUMBNAIL_H_

#include <QRectF>
#include "ThumbnailBase.h"

class QSizeF;
class ThumbnailPixmapCache;
class ImageId;
class ImageTransformation;

namespace select_content {
class Thumbnail : public ThumbnailBase {
 public:
  Thumbnail(intrusive_ptr<ThumbnailPixmapCache> thumbnail_cache,
            const QSizeF& max_size,
            const ImageId& image_id,
            const ImageTransformation& xform,
            const QRectF& content_rect,
            const QRectF& page_rect,
            bool page_rect_enabled,
            bool deviant);

  void paintOverImage(QPainter& painter,
                      const QTransform& image_to_display,
                      const QTransform& thumb_to_display) override;

 private:
  QRectF m_contentRect;
  QRectF m_pageRect;
  bool m_pageRectEnabled;
  bool m_deviant;
};
}  // namespace select_content
#endif
