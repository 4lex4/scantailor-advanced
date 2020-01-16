// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "FillColorProperty.h"

#include <QDomDocument>

#include "PropertyFactory.h"

namespace output {
const char FillColorProperty::m_propertyName[] = "FillColorProperty";

FillColorProperty::FillColorProperty(const QDomElement& el) : m_rgb(rgbFromString(el.attribute("color"))) {}

void FillColorProperty::registerIn(PropertyFactory& factory) {
  factory.registerProperty(m_propertyName, &FillColorProperty::construct);
}

intrusive_ptr<Property> FillColorProperty::clone() const {
  return make_intrusive<FillColorProperty>(*this);
}

QDomElement FillColorProperty::toXml(QDomDocument& doc, const QString& name) const {
  QDomElement el(doc.createElement(name));
  el.setAttribute("type", m_propertyName);
  el.setAttribute("color", rgbToString(m_rgb));
  return el;
}

intrusive_ptr<Property> FillColorProperty::construct(const QDomElement& el) {
  return make_intrusive<FillColorProperty>(el);
}

QRgb FillColorProperty::rgbFromString(const QString& str) {
  return QColor(str).rgb();
}

QString FillColorProperty::rgbToString(QRgb rgb) {
  return QColor(rgb).name();
}

FillColorProperty::FillColorProperty(const QColor& color) : m_rgb(color.rgb()) {}

QColor FillColorProperty::color() const {
  return QColor(m_rgb);
}

void FillColorProperty::setColor(const QColor& color) {
  m_rgb = color.rgb();
}
}  // namespace output