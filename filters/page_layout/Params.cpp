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

#include "Params.h"
#include "XmlMarshaller.h"
#include "XmlUnmarshaller.h"

namespace page_layout {
Params::Params(const Margins& hard_margins_mm,
               const QRectF& page_rect,
               const QRectF& content_rect,
               const QSizeF& content_size_mm,
               const Alignment& alignment,
               const bool auto_margins)
    : m_hardMarginsMM(hard_margins_mm),
      m_pageRect(page_rect),
      m_contentRect(content_rect),
      m_contentSizeMM(content_size_mm),
      m_alignment(alignment),
      m_autoMargins(auto_margins) {}

Params::Params(const QDomElement& el)
    : m_hardMarginsMM(XmlUnmarshaller::margins(el.namedItem("hardMarginsMM").toElement())),
      m_pageRect(XmlUnmarshaller::rectF(el.namedItem("pageRect").toElement())),
      m_contentRect(XmlUnmarshaller::rectF(el.namedItem("contentRect").toElement())),
      m_contentSizeMM(XmlUnmarshaller::sizeF(el.namedItem("contentSizeMM").toElement())),
      m_alignment(el.namedItem("alignment").toElement()),
      m_autoMargins(el.attribute("autoMargins") == "1") {}

QDomElement Params::toXml(QDomDocument& doc, const QString& name) const {
  XmlMarshaller marshaller(doc);

  QDomElement el(doc.createElement(name));
  el.appendChild(marshaller.margins(m_hardMarginsMM, "hardMarginsMM"));
  el.appendChild(marshaller.rectF(m_pageRect, "pageRect"));
  el.appendChild(marshaller.rectF(m_contentRect, "contentRect"));
  el.appendChild(marshaller.sizeF(m_contentSizeMM, "contentSizeMM"));
  el.appendChild(m_alignment.toXml(doc, "alignment"));
  el.setAttribute("autoMargins", m_autoMargins ? "1" : "0");

  return el;
}

const Margins& Params::hardMarginsMM() const {
  return m_hardMarginsMM;
}

const QRectF& Params::contentRect() const {
  return m_contentRect;
}

const QRectF& Params::pageRect() const {
  return m_pageRect;
}

const QSizeF& Params::contentSizeMM() const {
  return m_contentSizeMM;
}

const Alignment& Params::alignment() const {
  return m_alignment;
}

bool Params::isAutoMarginsEnabled() const {
  return m_autoMargins;
}
}  // namespace page_layout
