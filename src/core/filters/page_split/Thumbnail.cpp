// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "Thumbnail.h"

#include <core/IconProvider.h>

#include <QPainter>
#include <utility>

namespace page_split {
Thumbnail::Thumbnail(intrusive_ptr<ThumbnailPixmapCache> thumbnailCache,
                     const QSizeF& maxSize,
                     const ImageId& imageId,
                     const ImageTransformation& xform,
                     const PageLayout& layout,
                     bool leftHalfRemoved,
                     bool rightHalfRemoved)
    : ThumbnailBase(std::move(thumbnailCache), maxSize, imageId, xform),
      m_layout(layout),
      m_leftHalfRemoved(leftHalfRemoved),
      m_rightHalfRemoved(rightHalfRemoved) {
  if (leftHalfRemoved || rightHalfRemoved) {
    m_trashPixmap = IconProvider::getInstance().getIcon("trashed").pixmap(64, 64);
  }
}

void Thumbnail::prePaintOverImage(QPainter& painter,
                                  const QTransform& imageToDisplay,
                                  const QTransform& thumbToDisplay) {
  const QRectF canvasRect(imageXform().resultingRect());

  painter.setRenderHint(QPainter::Antialiasing, false);
  painter.setWorldTransform(imageToDisplay);

  painter.setPen(Qt::NoPen);
  switch (m_layout.type()) {
    case PageLayout::SINGLE_PAGE_UNCUT:
      painter.setBrush(QColor(0, 0, 255, 50));
      painter.drawRect(canvasRect);
      return;  // No split line will be drawn.
    case PageLayout::SINGLE_PAGE_CUT:
      painter.setBrush(QColor(0, 0, 255, 50));
      painter.drawPolygon(m_layout.singlePageOutline());
      break;
    case PageLayout::TWO_PAGES: {
      const QPolygonF leftPoly(m_layout.leftPageOutline());
      const QPolygonF rightPoly(m_layout.rightPageOutline());
      painter.setBrush(m_leftHalfRemoved ? QColor(0, 0, 0, 80) : QColor(0, 0, 255, 50));
      painter.drawPolygon(leftPoly);
      painter.setBrush(m_rightHalfRemoved ? QColor(0, 0, 0, 80) : QColor(255, 0, 0, 50));
      painter.drawPolygon(rightPoly);
      // Draw the trash icon.
      if (m_leftHalfRemoved || m_rightHalfRemoved) {
        painter.setWorldTransform(QTransform());

        const int subpageIdx = m_leftHalfRemoved ? 0 : 1;
        const QPointF center(subPageCenter(leftPoly, rightPoly, imageToDisplay, subpageIdx));

        QRectF rect(m_trashPixmap.rect());
        rect.moveCenter(center);
        painter.drawPixmap(rect.topLeft(), m_trashPixmap);

        painter.setWorldTransform(imageToDisplay);
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

QPointF Thumbnail::subPageCenter(const QPolygonF& leftPage,
                                 const QPolygonF& rightPage,
                                 const QTransform& imageToDisplay,
                                 int subpageIdx) {
  QRectF rects[2];
  rects[0] = leftPage.boundingRect();
  rects[1] = rightPage.boundingRect();

  const double xMid = 0.5 * (rects[0].right() + rects[1].left());
  rects[0].setRight(xMid);
  rects[1].setLeft(xMid);
  return imageToDisplay.map(rects[subpageIdx].center());
}
}  // namespace page_split