// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "PixmapRenderer.h"

#include <QPainter>
#include <QPixmap>
#include <QRect>
#include <QRectF>
#include <QTransform>

void PixmapRenderer::drawPixmap(QPainter& painter, const QPixmap& pixmap) {
  const QTransform invTransform(painter.worldTransform().inverted());
  const QRectF srcRect(invTransform.map(QRectF(painter.viewport())).boundingRect());
  const QRectF boundedSrcRect(srcRect.intersected(pixmap.rect()));
  painter.drawPixmap(boundedSrcRect, pixmap, boundedSrcRect);
}
