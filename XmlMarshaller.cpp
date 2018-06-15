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

#include "XmlMarshaller.h"
#include "Dpi.h"
#include "Margins.h"
#include "OrthogonalRotation.h"
#include "Utils.h"

QDomElement XmlMarshaller::string(const QString& str, const QString& name) {
  QDomElement el(m_doc.createElement(name));
  el.appendChild(m_doc.createTextNode(str));

  return el;
}

QDomElement XmlMarshaller::size(const QSize& size, const QString& name) {
  if (size.isNull()) {
    return QDomElement();
  }

  QDomElement el(m_doc.createElement(name));
  el.setAttribute("width", size.width());
  el.setAttribute("height", size.height());

  return el;
}

QDomElement XmlMarshaller::sizeF(const QSizeF& size, const QString& name) {
  if (size.isNull()) {
    return QDomElement();
  }

  QDomElement el(m_doc.createElement(name));
  el.setAttribute("width", Utils::doubleToString(size.width()));
  el.setAttribute("height", Utils::doubleToString(size.height()));

  return el;
}

QDomElement XmlMarshaller::dpi(const Dpi& dpi, const QString& name) {
  if (dpi.isNull()) {
    return QDomElement();
  }

  QDomElement el(m_doc.createElement(name));
  el.setAttribute("horizontal", dpi.horizontal());
  el.setAttribute("vertical", dpi.vertical());

  return el;
}

QDomElement XmlMarshaller::rotation(const OrthogonalRotation& rotation, const QString& name) {
  QDomElement el(m_doc.createElement(name));
  el.setAttribute("degrees", rotation.toDegrees());

  return el;
}

QDomElement XmlMarshaller::pointF(const QPointF& p, const QString& name) {
  QDomElement el(m_doc.createElement(name));
  el.setAttribute("x", Utils::doubleToString(p.x()));
  el.setAttribute("y", Utils::doubleToString(p.y()));

  return el;
}

QDomElement XmlMarshaller::lineF(const QLineF& line, const QString& name) {
  QDomElement el(m_doc.createElement(name));
  el.appendChild(pointF(line.p1(), "p1"));
  el.appendChild(pointF(line.p2(), "p2"));

  return el;
}

QDomElement XmlMarshaller::rect(const QRect& rect, const QString& name) {
  QDomElement el(m_doc.createElement(name));
  el.setAttribute("x", QString::number(rect.x()));
  el.setAttribute("y", QString::number(rect.y()));
  el.setAttribute("width", QString::number(rect.width()));
  el.setAttribute("height", QString::number(rect.height()));

  return el;
}

QDomElement XmlMarshaller::rectF(const QRectF& rect, const QString& name) {
  QDomElement el(m_doc.createElement(name));
  el.setAttribute("x", Utils::doubleToString(rect.x()));
  el.setAttribute("y", Utils::doubleToString(rect.y()));
  el.setAttribute("width", Utils::doubleToString(rect.width()));
  el.setAttribute("height", Utils::doubleToString(rect.height()));

  return el;
}

QDomElement XmlMarshaller::polygonF(const QPolygonF& poly, const QString& name) {
  QDomElement el(m_doc.createElement(name));

  QPolygonF::const_iterator it(poly.begin());
  const QPolygonF::const_iterator end(poly.end());
  for (; it != end; ++it) {
    el.appendChild(pointF(*it, "point"));
  }

  return el;
}

QDomElement XmlMarshaller::margins(const Margins& margins, const QString& name) {
  QDomElement el(m_doc.createElement(name));
  el.setAttribute("left", Utils::doubleToString(margins.left()));
  el.setAttribute("right", Utils::doubleToString(margins.right()));
  el.setAttribute("top", Utils::doubleToString(margins.top()));
  el.setAttribute("bottom", Utils::doubleToString(margins.bottom()));

  return el;
}
