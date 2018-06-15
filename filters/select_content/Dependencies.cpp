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

#include "Dependencies.h"
#include "XmlMarshaller.h"
#include "XmlUnmarshaller.h"
#include "imageproc/PolygonUtils.h"

using namespace imageproc;

namespace select_content {
Dependencies::Dependencies() : m_invalid(false) {}

Dependencies::Dependencies(const QPolygonF& rotated_page_outline)
    : m_rotatedPageOutline(rotated_page_outline), m_invalid(false) {}

Dependencies::Dependencies(const QDomElement& deps_el)
    : m_rotatedPageOutline(XmlUnmarshaller::polygonF(deps_el.namedItem("rotated-page-outline").toElement())),
      m_invalid(deps_el.attribute("invalid") == "1") {}

Dependencies::~Dependencies() = default;

bool Dependencies::matches(const Dependencies& other) const {
  if (m_invalid) {
    return false;
  }

  return PolygonUtils::fuzzyCompare(m_rotatedPageOutline, other.m_rotatedPageOutline);
}

QDomElement Dependencies::toXml(QDomDocument& doc, const QString& name) const {
  XmlMarshaller marshaller(doc);

  QDomElement el(doc.createElement(name));
  el.appendChild(marshaller.polygonF(m_rotatedPageOutline, "rotated-page-outline"));
  el.setAttribute("invalid", m_invalid ? "1" : "0");

  return el;
}

void Dependencies::invalidate() {
  m_invalid = true;
}

const QPolygonF& Dependencies::rotatedPageOutline() const {
  return m_rotatedPageOutline;
}
}  // namespace select_content