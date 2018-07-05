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

#ifndef OUTPUT_ZONE_CATEGORY_PROPERTY_H_
#define OUTPUT_ZONE_CATEGORY_PROPERTY_H_

#include "Property.h"
#include "intrusive_ptr.h"

class PropertyFactory;
class QDomDocument;
class QDomElement;
class QString;

namespace output {
class ZoneCategoryProperty : public Property {
 public:
  enum ZoneCategory { MANUAL, RECTANGULAR_OUTLINE };

  explicit ZoneCategoryProperty(ZoneCategory zone_category = MANUAL) : m_zone_category(zone_category) {}

  explicit ZoneCategoryProperty(const QDomElement& el);

  static void registerIn(PropertyFactory& factory);

  intrusive_ptr<Property> clone() const override;

  QDomElement toXml(QDomDocument& doc, const QString& name) const override;

  ZoneCategory zone_category() const;

  void setZoneCategory(ZoneCategory zone_category);

 private:
  static intrusive_ptr<Property> construct(const QDomElement& el);

  static ZoneCategory zoneCategoryFromString(const QString& str);

  static QString zoneCategoryToString(ZoneCategory zone_category);


  static const char m_propertyName[];
  ZoneCategory m_zone_category;
};
}  // namespace output

#endif  // ifndef OUTPUT_ZONE_CATEGORY_PROPERTY_H_
