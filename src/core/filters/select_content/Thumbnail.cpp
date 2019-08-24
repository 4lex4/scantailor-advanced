// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "Thumbnail.h"
#include <QPainter>
#include <utility>

namespace select_content {
Thumbnail::Thumbnail(intrusive_ptr<ThumbnailPixmapCache> thumbnailCache,
                     const QSizeF& maxSize,
                     const ImageId& imageId,
                     const ImageTransformation& xform,
                     const QRectF& contentRect,
                     const QRectF& pageRect,
                     bool pageRectEnabled,
                     bool deviant)
    : ThumbnailBase(std::move(thumbnailCache), maxSize, imageId, xform),
      m_contentRect(contentRect),
      m_pageRect(pageRect),
      m_pageRectEnabled(pageRectEnabled),
      m_deviant(deviant) {}

void Thumbnail::paintOverImage(QPainter& painter, const QTransform& imageToDisplay, const QTransform& thumbToDisplay) {
  if (!m_contentRect.isNull()) {
    QRectF pageRect(virtToThumb().mapRect(m_pageRect));

    painter.setRenderHint(QPainter::Antialiasing, false);

    if (m_pageRectEnabled) {
      QPen pen(QColor(0xff, 0x7f, 0x00));
      pen.setWidth(1);
      pen.setCosmetic(true);
      painter.setPen(pen);

      painter.setBrush(Qt::NoBrush);

      painter.drawRect(pageRect);
    }

    QPen pen(QColor(0x00, 0x00, 0xff));
    pen.setWidth(1);
    pen.setCosmetic(true);
    painter.setPen(pen);

    painter.setBrush(QColor(0x00, 0x00, 0xff, 50));

    QRectF contentRect(virtToThumb().mapRect(m_contentRect));

    // Adjust to compensate for pen width.
    contentRect.adjust(-1, -1, 1, 1);
    contentRect = contentRect.intersected(pageRect);

    painter.drawRect(contentRect);
  }

  if (m_deviant) {
    paintDeviant(painter);
  }
}
}  // namespace select_content