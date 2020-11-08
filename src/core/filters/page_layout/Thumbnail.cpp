// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "Thumbnail.h"

#include <PolygonUtils.h>

#include <QPainter>
#include <QPainterPath>
#include <utility>

#include "Utils.h"

using namespace imageproc;

namespace page_layout {
Thumbnail::Thumbnail(std::shared_ptr<ThumbnailPixmapCache> thumbnailCache,
                     const QSizeF& maxSize,
                     const ImageId& imageId,
                     const Params& params,
                     const ImageTransformation& xform,
                     const QPolygonF& physContentRect,
                     bool deviant)
    : ThumbnailBase(std::move(thumbnailCache), maxSize, imageId, xform, xform.resultingRect()),
      m_params(params),
      m_virtContentRect(xform.transform().map(physContentRect).boundingRect()),
      m_virtOuterRect(xform.resultingRect()),
      m_deviant(deviant) {}

void Thumbnail::paintOverImage(QPainter& painter, const QTransform& imageToDisplay, const QTransform& thumbToDisplay) {
  // We work in display coordinates because we want to be
  // pixel-accurate with what we draw.
  painter.setWorldTransform(QTransform());

  const QTransform virtToDisplay(virtToThumb() * thumbToDisplay);

  const QRectF innerRect(virtToDisplay.map(m_virtContentRect).boundingRect());

  // We extend the outer rectangle because otherwise we may get white
  // thin lines near the edges due to rounding errors and the lack
  // of subpixel accuracy.  Doing that is actually OK, because what
  // we paint will be clipped anyway.
  const QRectF outerRect(virtToDisplay.map(m_virtOuterRect).boundingRect());

  QPainterPath outerOutline;
  outerOutline.addPolygon(outerRect);

  QPainterPath contentOutline;
  contentOutline.addPolygon(innerRect);

  painter.setRenderHint(QPainter::Antialiasing, true);

  QColor bgColor;
  QColor fgColor;
  if (m_params.alignment().isNull()) {
    // "Align with other pages" is turned off.
    // Different color is useful on a thumbnail list to
    // distinguish "safe" pages from potentially problematic ones.
    bgColor = QColor(0x58, 0x7f, 0xf4, 70);
    fgColor = QColor(0x00, 0x52, 0xff);
  } else {
    bgColor = QColor(0xbb, 0x00, 0xff, 40);
    fgColor = QColor(0xbe, 0x5b, 0xec);
  }

  // we round inner rect to check whether content rect is empty and this is just an adapted rect.
  const bool isNullContentRect = m_virtContentRect.toRect().isEmpty();

  // Draw margins.
  if (!isNullContentRect) {
    painter.fillPath(outerOutline.subtracted(contentOutline), bgColor);
  } else {
    painter.fillPath(outerOutline, bgColor);
  }

  QPen pen(fgColor);
  pen.setCosmetic(true);
  pen.setWidthF(1.0);
  painter.setPen(pen);
  painter.setBrush(Qt::NoBrush);

  // inner rect
  if (!isNullContentRect) {
    painter.drawRect(innerRect);
  }

  // outer rect
  if (!m_params.alignment().isNull()) {
    pen.setStyle(Qt::DashLine);
  }
  painter.setPen(pen);
  painter.drawRect(outerRect);

  if (m_deviant) {
    painter.setWorldTransform(thumbToDisplay);
    paintDeviant(painter);
  }
}  // Thumbnail::paintOverImage
}  // namespace page_layout