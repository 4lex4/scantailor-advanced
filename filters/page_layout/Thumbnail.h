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
