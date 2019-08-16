// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "Thumbnail.h"
#include <QPainter>
#include <utility>

namespace select_content {
Thumbnail::Thumbnail(intrusive_ptr<ThumbnailPixmapCache> thumbnail_cache,
                     const QSizeF& max_size,
                     const ImageId& image_id,
                     const ImageTransformation& xform,
                     const QRectF& content_rect,
                     const QRectF& page_rect,
                     bool page_rect_enabled,
                     bool deviant)
    : ThumbnailBase(std::move(thumbnail_cache), max_size, image_id, xform),
      m_contentRect(content_rect),
      m_pageRect(page_rect),
      m_pageRectEnabled(page_rect_enabled),
      m_deviant(deviant) {}

void Thumbnail::paintOverImage(QPainter& painter,
                               const QTransform& image_to_display,
                               const QTransform& thumb_to_display) {
  if (!m_contentRect.isNull()) {
    QRectF page_rect(virtToThumb().mapRect(m_pageRect));

    painter.setRenderHint(QPainter::Antialiasing, false);

    if (m_pageRectEnabled) {
      QPen pen(QColor(0xff, 0x7f, 0x00));
      pen.setWidth(1);
      pen.setCosmetic(true);
      painter.setPen(pen);

      painter.setBrush(Qt::NoBrush);

      painter.drawRect(page_rect);
    }

    QPen pen(QColor(0x00, 0x00, 0xff));
    pen.setWidth(1);
    pen.setCosmetic(true);
    painter.setPen(pen);

    painter.setBrush(QColor(0x00, 0x00, 0xff, 50));

    QRectF content_rect(virtToThumb().mapRect(m_contentRect));

    // Adjust to compensate for pen width.
    content_rect.adjust(-1, -1, 1, 1);
    content_rect = content_rect.intersected(page_rect);

    painter.drawRect(content_rect);
  }

  if (m_deviant) {
    paintDeviant(painter);
  }
}
}  // namespace select_content