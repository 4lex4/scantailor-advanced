// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

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
  enum ZoneCategory { MANUAL, AUTO };

  explicit ZoneCategoryProperty(ZoneCategory zone_category = MANUAL);

  ZoneCategoryProperty(const ZoneCategoryProperty& other);

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
  ZoneCategory m_zoneCategory;
};
}  // namespace output

#endif  // ifndef OUTPUT_ZONE_CATEGORY_PROPERTY_H_
