/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2007-2015  Joseph Artsimovich <joseph.artsimovich@gmail.com>

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
