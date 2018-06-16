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

#ifndef ZONE_H_
#define ZONE_H_

#include "PropertySet.h"
#include "SerializableSpline.h"
#include "filters/output/PictureLayerProperty.h"
#include "filters/output/ZoneCategoryProperty.h"
#include "intrusive_ptr.h"

class PropertyFactory;
class QDomDocument;
class QDomElement;
class QString;

class Zone {
  // Member-wise copying is OK, but that will produce a partly shallow copy.
 public:
  explicit Zone(const SerializableSpline& spline, const PropertySet& props = PropertySet());

  Zone(const QDomElement& el, const PropertyFactory& prop_factory);

  explicit Zone(const QPolygonF& polygon);

  QDomElement toXml(QDomDocument& doc, const QString& name) const;

  const SerializableSpline& spline() const { return m_spline; }

  PropertySet& properties() { return m_props; }

  const PropertySet& properties() const { return m_props; }

  bool isValid() const;

 private:
  SerializableSpline m_spline;
  PropertySet m_props;
};


#endif  // ifndef ZONE_H_
