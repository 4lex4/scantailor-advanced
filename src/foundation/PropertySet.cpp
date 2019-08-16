// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

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
    intrusive_ptr<Property> prop = factory.construct(prop_el);
    if (prop) {
      m_props[typeid(*prop)] = prop;
    }
  }
}

PropertySet::PropertySet(const PropertySet& other) : ref_countable(other) {
  m_props.reserve(other.m_props.size());
  for (const auto& [type, prop] : other.m_props) {
    m_props[type] = prop->clone();
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

  for (const auto& [type, prop] : m_props) {
    props_el.appendChild(prop->toXml(doc, property_str));
  }

  return props_el;
}
