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
