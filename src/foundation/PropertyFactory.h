// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_FOUNDATION_PROPERTYFACTORY_H_
#define SCANTAILOR_FOUNDATION_PROPERTYFACTORY_H_

#include <QString>
#include <unordered_map>
#include "Hashes.h"
#include "Property.h"
#include "intrusive_ptr.h"

class QDomElement;

class PropertyFactory {
  // Member-wise copying is OK.
 public:
  virtual ~PropertyFactory() = default;

  using PropertyConstructor = intrusive_ptr<Property> (*)(const QDomElement& el);

  void registerProperty(const QString& property, PropertyConstructor constructor);

  intrusive_ptr<Property> construct(const QDomElement& el) const;

 private:
  typedef std::unordered_map<QString, PropertyConstructor, hashes::hash<QString>> Registry;
  Registry m_registry;
};


#endif
