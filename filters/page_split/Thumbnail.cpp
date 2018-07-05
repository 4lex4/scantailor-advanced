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

#include "Thumbnail.h"
#include <QPainter>
#include <utility>

namespace page_split {
Thumbnail::Thumbnail(intrusive_ptr<ThumbnailPixmapCache> thumbnail_cache,
                     const QSizeF& max_size,
                     const ImageId& image_id,
                     const ImageTransformation& xform,
                     const PageLayout& layout,
                     bool left_half_removed,
                     bool right_half_removed)
    : ThumbnailBase(std::move(thumbnail_cache), max_size, image_id, xform),
      m_layout(layout),
      m_leftHalfRemoved(left_half_removed),
      m_rightHalfRemoved(right_half_removed) {
  if (left_half_removed || right_half_removed) {
    m_trashPixmap = QPixmap(":/icons/trashed-small.png");
  }
}

void Thumbnail::prePaintOverImage(QPainter& painter,
                                  const QTransform& image_to_display,
                                  const QTransform& thumb_to_display) {
  const QRectF canvas_rect(imageXform().resultingRect());

  painter.setRenderHint(QPainter::Antialiasing, false);
  painter.setWorldTransform(image_to_display);

  painter.setPen(Qt::NoPen);
  switch (m_layout.type()) {
    case PageLayout::SINGLE_PAGE_UNCUT:
      painter.setBrush(QColor(0, 0, 255, 50));
      painter.drawRect(canvas_rect);

      return;  // No split line will be drawn.
    case PageLayout::SINGLE_PAGE_CUT:
      painter.setBrush(QColor(0, 0, 255, 50));
      painter.drawPolygon(m_layout.singlePageOutline());
      break;
    case PageLayout::TWO_PAGES: {
      const QPolygonF left_poly(m_layout.leftPageOutline());
      const QPolygonF right_poly(m_layout.rightPageOutline());
      painter.setBrush(m_leftHalfRemoved ? QColor(0, 0, 0, 80) : QColor(0, 0, 255, 50));
      painter.drawPolygon(left_poly);
      painter.setBrush(m_rightHalfRemoved ? QColor(0, 0, 0, 80) : QColor(255, 0, 0, 50));
      painter.drawPolygon(right_poly);
      // Draw the trash icon.
      if (m_leftHalfRemoved || m_rightHalfRemoved) {
        painter.setWorldTransform(QTransform());

        const int subpage_idx = m_leftHalfRemoved ? 0 : 1;
        const QPointF center(subPageCenter(left_poly, right_poly, image_to_display, subpage_idx));

        QRectF rect(m_trashPixmap.rect());
        rect.moveCenter(center);
        painter.drawPixmap(rect.topLeft(), m_trashPixmap);

        painter.setWorldTransform(image_to_display);
      }
      break;
    }
  }  // switch

  painter.setRenderHint(QPainter::Antialiasing, true);

  QPen pen(QColor(0x00, 0x00, 0xff));
  pen.setWidth(1);
  pen.setCosmetic(true);
  painter.setPen(pen);

  switch (m_layout.type()) {
    case PageLayout::SINGLE_PAGE_CUT:
      painter.drawLine(m_layout.inscribedCutterLine(0));
      painter.drawLine(m_layout.inscribedCutterLine(1));
      break;
    case PageLayout::TWO_PAGES:
      painter.drawLine(m_layout.inscribedCutterLine(0));
      break;
    default:;
  }
}  // Thumbnail::paintOverImage

QPointF Thumbnail::subPageCenter(const QPolygonF& left_page,
                                 const QPolygonF& right_page,
                                 const QTransform& image_to_display,
                                 int subpage_idx) {
  QRectF rects[2];
  rects[0] = left_page.boundingRect();
  rects[1] = right_page.boundingRect();

  const double x_mid = 0.5 * (rects[0].right() + rects[1].left());
  rects[0].setRight(x_mid);
  rects[1].setLeft(x_mid);

  return image_to_display.map(rects[subpage_idx].center());
}
}  // namespace page_split