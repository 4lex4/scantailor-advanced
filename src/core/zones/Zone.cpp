// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "Zone.h"

#include <QDomDocument>

Zone::Zone(const SerializableSpline& spline, const PropertySet& props) : m_spline(spline), m_props(props) {}

Zone::Zone(const QDomElement& el, const PropertyFactory& propFactory)
    : m_spline(el.namedItem("spline").toElement()), m_props(el.namedItem("properties").toElement(), propFactory) {}

QDomElement Zone::toXml(QDomDocument& doc, const QString& name) const {
  QDomElement el(doc.createElement(name));
  el.appendChild(m_spline.toXml(doc, "spline"));
  el.appendChild(m_props.toXml(doc, "properties"));
  return el;
}

bool Zone::isValid() const {
  const QPolygonF& shape = m_spline.toPolygon();

  switch (shape.size()) {
    case 0:
    case 1:
    case 2:
      return false;
    case 3:
      if (shape.front() == shape.back()) {
        return false;
      }
      // fall through
    default:
      return true;
  }
}

const SerializableSpline& Zone::spline() const {
  return m_spline;
}

PropertySet& Zone::properties() {
  return m_props;
}

const PropertySet& Zone::properties() const {
  return m_props;
}
