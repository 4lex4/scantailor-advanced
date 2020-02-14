// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_ZONES_ZONE_H_
#define SCANTAILOR_ZONES_ZONE_H_

#include <PropertySet.h>

#include "SerializableSpline.h"

class PropertyFactory;
class QDomDocument;
class QDomElement;
class QString;

class Zone {
  // Member-wise copying is OK, but that will produce a partly shallow copy.
 public:
  explicit Zone(const SerializableSpline& spline, const PropertySet& props = PropertySet());

  Zone(const QDomElement& el, const PropertyFactory& propFactory);

  QDomElement toXml(QDomDocument& doc, const QString& name) const;

  const SerializableSpline& spline() const;

  PropertySet& properties();

  const PropertySet& properties() const;

  bool isValid() const;

 private:
  SerializableSpline m_spline;
  PropertySet m_props;
};


inline const SerializableSpline& Zone::spline() const {
  return m_spline;
}

inline PropertySet& Zone::properties() {
  return m_props;
}

inline const PropertySet& Zone::properties() const {
  return m_props;
}

#endif  // ifndef SCANTAILOR_ZONES_ZONE_H_
