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

#ifndef OUTPUT_FILL_COLOR_PROPERTY_H_
#define OUTPUT_FILL_COLOR_PROPERTY_H_

#include <QColor>
#include <Qt>
#include "Property.h"
#include "intrusive_ptr.h"

class PropertyFactory;
class QDomDocument;
class QDomElement;
class QString;

namespace output {
class FillColorProperty : public Property {
 public:
  explicit FillColorProperty(const QColor& color = Qt::white);

  explicit FillColorProperty(const QDomElement& el);

  static void registerIn(PropertyFactory& factory);

  intrusive_ptr<Property> clone() const override;

  QDomElement toXml(QDomDocument& doc, const QString& name) const override;

  QColor color() const;

  void setColor(const QColor& color);

 private:
  static intrusive_ptr<Property> construct(const QDomElement& el);

  static QRgb rgbFromString(const QString& str);

  static QString rgbToString(QRgb rgb);


  static const char m_propertyName[];
  QRgb m_rgb;
};
}  // namespace output
#endif  // ifndef OUTPUT_FILL_COLOR_PROPERTY_H_
