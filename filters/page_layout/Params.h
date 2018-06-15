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

#include <QRectF>
#include <QSizeF>
#include "Alignment.h"
#include "Margins.h"

class QDomDocument;
class QDomElement;
class QString;

namespace page_layout {
class Params {
  // Member-wise copying is OK.
 public:
  Params(const Margins& hard_margins_mm,
         const QRectF& page_rect,
         const QRectF& content_rect,
         const QSizeF& content_size_mm,
         const Alignment& alignment,
         bool auto_margins);

  explicit Params(const QDomElement& el);

  const Margins& hardMarginsMM() const;

  const QRectF& contentRect() const;

  const QRectF& pageRect() const;

  const QSizeF& contentSizeMM() const;

  const Alignment& alignment() const;

  bool isAutoMarginsEnabled() const;

  QDomElement toXml(QDomDocument& doc, const QString& name) const;

 private:
  Margins m_hardMarginsMM;
  QRectF m_contentRect;
  QRectF m_pageRect;
  QSizeF m_contentSizeMM;
  Alignment m_alignment;
  bool m_autoMargins;
};
}  // namespace page_layout
#endif  // ifndef PAGE_LAYOUT_PARAMS_H_
