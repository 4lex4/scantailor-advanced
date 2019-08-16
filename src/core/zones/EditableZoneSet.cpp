// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "EditableZoneSet.h"

EditableZoneSet::EditableZoneSet() : m_zoneItems(), m_zoneItemsInOrder(m_zoneItems.get<ZoneItemOrderedTag>()) {}

void EditableZoneSet::setDefaultProperties(const PropertySet& props) {
  m_defaultProps = props;
}

void EditableZoneSet::addZone(const EditableSpline::Ptr& spline) {
  auto new_props = make_intrusive<PropertySet>(m_defaultProps);
  m_zoneItems.insert(ZoneItem(spline, new_props));
}

void EditableZoneSet::addZone(const EditableSpline::Ptr& spline, const PropertySet& props) {
  auto new_props = make_intrusive<PropertySet>(props);
  m_zoneItems.insert(ZoneItem(spline, new_props));
}

void EditableZoneSet::removeZone(const EditableSpline::Ptr& spline) {
  m_zoneItems.erase(spline);
}

void EditableZoneSet::commit() {
  emit committed();
}

intrusive_ptr<PropertySet> EditableZoneSet::propertiesFor(const EditableSpline::Ptr& spline) {
  auto it(m_zoneItems.find(spline));
  if (it != m_zoneItems.end()) {
    return it->second;
  } else {
    return nullptr;
  }
}

intrusive_ptr<const PropertySet> EditableZoneSet::propertiesFor(const EditableSpline::Ptr& spline) const {
  auto it(m_zoneItems.find(spline));
  if (it != m_zoneItems.end()) {
    return it->second;
  } else {
    return nullptr;
  }
}
