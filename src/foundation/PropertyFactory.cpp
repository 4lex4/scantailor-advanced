// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "PropertyFactory.h"
#include <QDomElement>

void PropertyFactory::registerProperty(const QString& property, PropertyConstructor constructor) {
  m_registry[property] = constructor;
}

intrusive_ptr<Property> PropertyFactory::construct(const QDomElement& el) const {
  auto it(m_registry.find(el.attribute("type")));
  if (it != m_registry.end()) {
    return (*it->second)(el);
  } else {
    return nullptr;
  }
}
