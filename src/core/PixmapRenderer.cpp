// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "PixmapRenderer.h"
#include <QPainter>
#include <QPixmap>
#include <QRect>
#include <QRectF>
#include <QTransform>

void PixmapRenderer::drawPixmap(QPainter& painter, const QPixmap& pixmap) {
  const QTransform inv_transform(painter.worldTransform().inverted());
  const QRectF src_rect(inv_transform.map(QRectF(painter.viewport())).boundingRect());
  const QRectF bounded_src_rect(src_rect.intersected(pixmap.rect()));
  painter.drawPixmap(bounded_src_rect, pixmap, bounded_src_rect);
}
