// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef ZONE_H_
#define ZONE_H_

#include "SerializableSpline.h"
#include <PropertySet.h>

class PropertyFactory;
class QDomDocument;
class QDomElement;
class QString;

class Zone {
  // Member-wise copying is OK, but that will produce a partly shallow copy.
 public:
  explicit Zone(const SerializableSpline& spline, const PropertySet& props = PropertySet());

  Zone(const QDomElement& el, const PropertyFactory& prop_factory);

  QDomElement toXml(QDomDocument& doc, const QString& name) const;

  const SerializableSpline& spline() const;

  PropertySet& properties();

  const PropertySet& properties() const;

  bool isValid() const;

 private:
  SerializableSpline m_spline;
  PropertySet m_props;
};


#endif  // ifndef ZONE_H_
