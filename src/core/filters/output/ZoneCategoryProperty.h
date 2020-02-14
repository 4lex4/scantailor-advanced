// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_OUTPUT_ZONECATEGORYPROPERTY_H_
#define SCANTAILOR_OUTPUT_ZONECATEGORYPROPERTY_H_

#include <memory>

#include "Property.h"

class PropertyFactory;
class QDomDocument;
class QDomElement;
class QString;

namespace output {
class ZoneCategoryProperty : public Property {
 public:
  enum ZoneCategory { MANUAL, AUTO };

  explicit ZoneCategoryProperty(ZoneCategory zoneCategory = MANUAL);

  explicit ZoneCategoryProperty(const QDomElement& el);

  static void registerIn(PropertyFactory& factory);

  std::shared_ptr<Property> clone() const override;

  QDomElement toXml(QDomDocument& doc, const QString& name) const override;

  ZoneCategory zoneCategory() const;

  void setZoneCategory(ZoneCategory zoneCategory);

 private:
  static std::shared_ptr<Property> construct(const QDomElement& el);

  static ZoneCategory zoneCategoryFromString(const QString& str);

  static QString zoneCategoryToString(ZoneCategory zoneCategory);


  static const char m_propertyName[];
  ZoneCategory m_zoneCategory;
};


inline ZoneCategoryProperty::ZoneCategory ZoneCategoryProperty::zoneCategory() const {
  return m_zoneCategory;
}

inline void ZoneCategoryProperty::setZoneCategory(ZoneCategoryProperty::ZoneCategory zoneCategory) {
  m_zoneCategory = zoneCategory;
}
}  // namespace output

#endif  // ifndef SCANTAILOR_OUTPUT_ZONECATEGORYPROPERTY_H_
