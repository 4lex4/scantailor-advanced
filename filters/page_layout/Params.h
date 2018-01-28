/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2007-2008  Joseph Artsimovich <joseph_a@mail.ru>

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

#ifndef PAGE_LAYOUT_PARAMS_H_
#define PAGE_LAYOUT_PARAMS_H_

#include "Margins.h"
#include "Alignment.h"
#include <QSizeF>
#include <QRectF>

class QDomDocument;
class QDomElement;
class QString;

namespace page_layout {
    class Params {
        // Member-wise copying is OK.
    public:
        Params(Margins const& hard_margins_mm,
               QRectF const& page_rect,
               QRectF const& content_rect,
               QSizeF const& content_size_mm,
               Alignment const& alignment,
               bool auto_margins);

        explicit Params(QDomElement const& el);

        Margins const& hardMarginsMM() const;

        QRectF const& contentRect() const;

        QRectF const& pageRect() const;

        QSizeF const& contentSizeMM() const;

        Alignment const& alignment() const;

        bool isAutoMarginsEnabled() const;

        bool isDeviant() const;

        QDomElement toXml(QDomDocument& doc, QString const& name) const;

    private:
        Margins m_hardMarginsMM;
        QRectF m_contentRect;
        QRectF m_pageRect;
        QSizeF m_contentSizeMM;
        Alignment m_alignment;
        bool m_autoMargins;
    };
}  // namespace page_layout
#endif // ifndef PAGE_LAYOUT_PARAMS_H_
