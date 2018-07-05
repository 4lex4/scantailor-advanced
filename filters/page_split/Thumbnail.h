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
