// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef PAGE_SPLIT_THUMBNAIL_H_
#define PAGE_SPLIT_THUMBNAIL_H_

#include <QPixmap>
#include "PageLayout.h"
#include "ThumbnailBase.h"
#include "intrusive_ptr.h"

class QPointF;
class QSizeF;
class QPolygonF;
class ThumbnailPixmapCache;
class ImageId;
class ImageTransformation;

namespace page_split {
class Thumbnail : public ThumbnailBase {
 public:
  Thumbnail(intrusive_ptr<ThumbnailPixmapCache> thumbnail_cache,
            const QSizeF& max_size,
            const ImageId& image_id,
            const ImageTransformation& xform,
            const PageLayout& layout,
            bool left_half_removed,
            bool right_half_removed);

  void prePaintOverImage(QPainter& painter,
                         const QTransform& image_to_display,
                         const QTransform& thumb_to_display) override;

 private:
  QPointF subPageCenter(const QPolygonF& left_page,
                        const QPolygonF& right_page,
                        const QTransform& image_to_display,
                        int subpage_idx);

  PageLayout m_layout;
  QPixmap m_trashPixmap;
  bool m_leftHalfRemoved;
  bool m_rightHalfRemoved;
};
}  // namespace page_split
#endif  // ifndef PAGE_SPLIT_THUMBNAIL_H_
