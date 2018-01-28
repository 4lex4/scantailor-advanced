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
#include "Utils.h"
#include "imageproc/PolygonUtils.h"
#include <QPainter>
#include <utility>

using namespace imageproc;

namespace page_layout {
    Thumbnail::Thumbnail(intrusive_ptr<ThumbnailPixmapCache> thumbnail_cache,
                         QSizeF const& max_size,
                         ImageId const& image_id,
                         Params const& params,
                         ImageTransformation const& xform,
                         QPolygonF const& phys_content_rect,
                         QRectF displayArea)
            : ThumbnailBase(std::move(thumbnail_cache), max_size, image_id, xform, displayArea),
              m_params(params),
              m_virtContentRect(xform.transform().map(phys_content_rect).boundingRect()),
              m_virtOuterRect(displayArea) {
        setExtendedClipArea(true);
    }

    void Thumbnail::paintOverImage(QPainter& painter, QTransform const& image_to_display,
                                   QTransform const& thumb_to_display) {
        // We work in display coordinates because we want to be
        // pixel-accurate with what we draw.
        painter.setWorldTransform(QTransform());

        QTransform const virt_to_display(virtToThumb() * thumb_to_display);

        QRectF const inner_rect(virt_to_display.map(m_virtContentRect).boundingRect());

        // We extend the outer rectangle because otherwise we may get white
        // thin lines near the edges due to rounding errors and the lack
        // of subpixel accuracy.  Doing that is actually OK, because what
        // we paint will be clipped anyway.
        QRectF const outer_rect(
                virt_to_display.map(m_virtOuterRect).boundingRect()
        );

        QPainterPath outer_outline;
        outer_outline.addPolygon(outer_rect);

        QPainterPath content_outline;
        content_outline.addPolygon(inner_rect);

        painter.setRenderHint(QPainter::Antialiasing, true);

        QColor bg_color;
        QColor fg_color;
        if (m_params.alignment().isNull()) {
            // "Align with other pages" is turned off.
            // Different color is useful on a thumbnail list to
            // distinguish "safe" pages from potentially problematic ones.
            bg_color = QColor(0x58, 0x7f, 0xf4, 70);
            fg_color = QColor(0x00, 0x52, 0xff);
        } else {
            bg_color = QColor(0xbb, 0x00, 0xff, 40);
            fg_color = QColor(0xbe, 0x5b, 0xec);
        }

        // we round inner rect to check whether content rect is empty and this is just an adapted rect.
        const bool isNullContentRect = m_virtContentRect.toRect().isEmpty();

        // Draw margins.
        if (!isNullContentRect) {
            painter.fillPath(outer_outline.subtracted(content_outline), bg_color);
        } else {
            painter.fillPath(outer_outline, bg_color);
        }

        QPen pen(fg_color);
        pen.setCosmetic(true);
        pen.setWidthF(1.0);
        painter.setPen(pen);
        painter.setBrush(Qt::NoBrush);

        // inner rect
        if (!isNullContentRect) {
            painter.drawRect(inner_rect);
        }

        // outer rect
        if (!m_params.alignment().isNull()) {
            pen.setStyle(Qt::DashLine);
        }
        painter.setPen(pen);
        painter.drawRect(outer_rect);

        if (m_params.isDeviant()) {
            paintDeviant(painter);
        }
    }  // Thumbnail::paintOverImage
}  // namespace page_layout