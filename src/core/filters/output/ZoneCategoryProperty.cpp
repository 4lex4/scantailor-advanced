// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "ZoneCategoryProperty.h"
#include <QDomDocument>
#include "PropertyFactory.h"

namespace output {
const char ZoneCategoryProperty::m_propertyName[] = "ZoneCategoryProperty";

ZoneCategoryProperty::ZoneCategoryProperty(ZoneCategoryProperty::ZoneCategory zone_category)
    : m_zoneCategory(zone_category) {}

ZoneCategoryProperty::ZoneCategoryProperty(const ZoneCategoryProperty& other) : m_zoneCategory(MANUAL) {}

ZoneCategoryProperty::ZoneCategoryProperty(const QDomElement& el)
    : m_zoneCategory(zoneCategoryFromString(el.attribute("zone_category"))) {}

void ZoneCategoryProperty::registerIn(PropertyFactory& factory) {
  factory.registerProperty(m_propertyName, &ZoneCategoryProperty::construct);
}

intrusive_ptr<Property> ZoneCategoryProperty::clone() const {
  return make_intrusive<ZoneCategoryProperty>(*this);
}

QDomElement ZoneCategoryProperty::toXml(QDomDocument& doc, const QString& name) const {
  QDomElement el(doc.createElement(name));
  el.setAttribute("type", m_propertyName);
  el.setAttribute("zone_category", zoneCategoryToString(m_zoneCategory));

  return el;
}

intrusive_ptr<Property> ZoneCategoryProperty::construct(const QDomElement& el) {
  return make_intrusive<ZoneCategoryProperty>(el);
}

ZoneCategoryProperty::ZoneCategory ZoneCategoryProperty::zoneCategoryFromString(const QString& str) {
  if (str == "rectangular_outline") {
    return AUTO;
  } else {
    return MANUAL;
  }
}

QString ZoneCategoryProperty::zoneCategoryToString(ZoneCategory zone_category) {
  QString str;

  switch (zone_category) {
    case MANUAL:
      str = "manual";
      break;
    case AUTO:
      str = "rectangular_outline";
      break;
    default:
      str = "";
      break;
  }

  return str;
}

ZoneCategoryProperty::ZoneCategory ZoneCategoryProperty::zone_category() const {
  return m_zoneCategory;
}

void ZoneCategoryProperty::setZoneCategory(ZoneCategoryProperty::ZoneCategory zone_category) {
  m_zoneCategory = zone_category;
}
}  // namespace output