// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "Thumbnail.h"
#include <QPainter>
#include <utility>

namespace deskew {
Thumbnail::Thumbnail(intrusive_ptr<ThumbnailPixmapCache> thumbnail_cache,
                     const QSizeF& max_size,
                     const ImageId& image_id,
                     const ImageTransformation& xform,
                     bool deviant)
    : ThumbnailBase(std::move(thumbnail_cache), max_size, image_id, xform), m_deviant(deviant) {}

void Thumbnail::prePaintOverImage(QPainter& painter,
                                  const QTransform& image_to_display,
                                  const QTransform& thumb_to_display) {
  painter.setRenderHint(QPainter::Antialiasing, false);

  QPen pen(QColor(0, 0, 0xd1, 70));
  pen.setWidth(1);
  pen.setCosmetic(true);
  painter.setPen(pen);

  const QRectF bounding_rect(boundingRect());

  const double cell_size = 8;
  const double left = bounding_rect.left();
  const double right = bounding_rect.right();
  const double top = bounding_rect.top();
  const double bottom = bounding_rect.bottom();
  const double w = bounding_rect.width();
  const double h = bounding_rect.height();

  const QPointF center(bounding_rect.center());
  QVector<QLineF> lines;
  for (double y = center.y(); y > 0.0; y -= cell_size) {
    lines.push_back(QLineF(left, y, right, y));
  }
  for (double y = center.y(); (y += cell_size) < h;) {
    lines.push_back(QLineF(left, y, right, y));
  }
  for (double x = center.x(); x > 0.0; x -= cell_size) {
    lines.push_back(QLineF(x, top, x, bottom));
  }
  for (double x = center.x(); (x += cell_size) < w;) {
    lines.push_back(QLineF(x, top, x, bottom));
  }
  painter.drawLines(lines);

  if (m_deviant) {
    paintDeviant(painter);
  }
}  // Thumbnail::paintOverImage
}  // namespace deskew