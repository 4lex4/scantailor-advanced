// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_FOUNDATION_PROPERTY_H_
#define SCANTAILOR_FOUNDATION_PROPERTY_H_

#include <memory>

class QDomDocument;
class QDomElement;
class QString;

class Property {
 public:
  virtual ~Property() = default;

  virtual std::shared_ptr<Property> clone() const = 0;

  virtual QDomElement toXml(QDomDocument& doc, const QString& name) const = 0;
};


#endif
