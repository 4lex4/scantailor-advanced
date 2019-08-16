// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef PROPERTY_H_
#define PROPERTY_H_

#include "intrusive_ptr.h"
#include "ref_countable.h"

class QDomDocument;
class QDomElement;

class Property : public ref_countable {
 public:
  virtual intrusive_ptr<Property> clone() const = 0;

  virtual QDomElement toXml(QDomDocument& doc, const QString& name) const = 0;
};


#endif
