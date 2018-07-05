/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C)  Joseph Artsimovich <joseph.artsimovich@gmail.com>

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