/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2007-2009  Joseph Artsimovich <joseph_a@mail.ru>

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

#include "PropertySet.h"
#include <QDomDocument>
#include "PropertyFactory.h"

PropertySet::PropertySet(const QDomElement& el, const PropertyFactory& factory) {
  const QString property_str("property");
  QDomNode node(el.firstChild());

  for (; !node.isNull(); node = node.nextSibling()) {
    if (!node.isElement()) {
      continue;
    }
    if (node.nodeName() != property_str) {
      continue;
    }

    QDomElement prop_el(node.toElement());
    intrusive_ptr<Property> prop(factory.construct(prop_el));
    if (prop) {
      m_props.push_back(prop);
    }
  }
}

PropertySet::PropertySet(const PropertySet& other) {
  m_props.reserve(other.m_props.size());

  for (const intrusive_ptr<Property>& prop : other.m_props) {
    m_props.push_back(prop->clone());
  }
}

PropertySet& PropertySet::operator=(const PropertySet& other) {
  PropertySet(other).swap(*this);

  return *this;
}

void PropertySet::swap(PropertySet& other) {
  m_props.swap(other.m_props);
}

QDomElement PropertySet::toXml(QDomDocument& doc, const QString& name) const {
  const QString property_str("property");
  QDomElement props_el(doc.createElement(name));

  for (const intrusive_ptr<Property>& prop : m_props) {
    props_el.appendChild(prop->toXml(doc, property_str));
  }

  return props_el;
}
