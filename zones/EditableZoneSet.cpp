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

#include "EditableZoneSet.h"

EditableZoneSet::EditableZoneSet() = default;

void EditableZoneSet::setDefaultProperties(const PropertySet& props) {
  m_defaultProps = props;
}

void EditableZoneSet::addZone(const EditableSpline::Ptr& spline) {
  auto new_props = make_intrusive<PropertySet>(m_defaultProps);
  m_splineMap.insert(Map::value_type(spline, new_props));
  m_splineList.push_back(spline);
}

void EditableZoneSet::addZone(const EditableSpline::Ptr& spline, const PropertySet& props) {
  auto new_props = make_intrusive<PropertySet>(props);
  m_splineMap.insert(Map::value_type(spline, new_props));
  m_splineList.push_back(spline);
}

void EditableZoneSet::removeZone(const EditableSpline::Ptr& spline) {
  m_splineMap.erase(spline);
  m_splineList.remove(spline);
}

void EditableZoneSet::commit() {
  emit committed();
}

intrusive_ptr<PropertySet> EditableZoneSet::propertiesFor(const EditableSpline::Ptr& spline) {
  auto it(m_splineMap.find(spline));
  if (it != m_splineMap.end()) {
    return it->second;
  } else {
    return nullptr;
  }
}

intrusive_ptr<const PropertySet> EditableZoneSet::propertiesFor(const EditableSpline::Ptr& spline) const {
  auto it(m_splineMap.find(spline));
  if (it != m_splineMap.end()) {
    return it->second;
  } else {
    return nullptr;
  }
}
