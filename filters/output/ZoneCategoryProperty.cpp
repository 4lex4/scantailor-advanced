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

#include "ZoneCategoryProperty.h"
#include <QDomDocument>
#include "PropertyFactory.h"

namespace output {
const char ZoneCategoryProperty::m_propertyName[] = "ZoneCategoryProperty";

ZoneCategoryProperty::ZoneCategoryProperty(const QDomElement& el)
    : m_zone_category(zoneCategoryFromString(el.attribute("zone_category"))) {}

void ZoneCategoryProperty::registerIn(PropertyFactory& factory) {
  factory.registerProperty(m_propertyName, &ZoneCategoryProperty::construct);
}

intrusive_ptr<Property> ZoneCategoryProperty::clone() const {
  return make_intrusive<ZoneCategoryProperty>(*this);
}

QDomElement ZoneCategoryProperty::toXml(QDomDocument& doc, const QString& name) const {
  QDomElement el(doc.createElement(name));
  el.setAttribute("type", m_propertyName);
  el.setAttribute("zone_category", zoneCategoryToString(m_zone_category));

  return el;
}

intrusive_ptr<Property> ZoneCategoryProperty::construct(const QDomElement& el) {
  return make_intrusive<ZoneCategoryProperty>(el);
}

ZoneCategoryProperty::ZoneCategory ZoneCategoryProperty::zoneCategoryFromString(const QString& str) {
  if (str == "manual") {
    return MANUAL;
  } else if (str == "rectangular_outline") {
    return RECTANGULAR_OUTLINE;
  } else {
    return MANUAL;
  }
}

QString ZoneCategoryProperty::zoneCategoryToString(ZoneCategory zone_category) {
  const char* str = nullptr;

  switch (zone_category) {
    case MANUAL:
      str = "manual";
      break;
    case RECTANGULAR_OUTLINE:
      str = "rectangular_outline";
      break;
    default:
      str = "";
      break;
  }

  return str;
}

ZoneCategoryProperty::ZoneCategory ZoneCategoryProperty::zone_category() const {
  return m_zone_category;
}

void ZoneCategoryProperty::setZoneCategory(ZoneCategoryProperty::ZoneCategory zone_category) {
  m_zone_category = zone_category;
}
}  // namespace output