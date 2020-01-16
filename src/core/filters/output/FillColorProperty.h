// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_OUTPUT_FILLCOLORPROPERTY_H_
#define SCANTAILOR_OUTPUT_FILLCOLORPROPERTY_H_

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
#endif  // ifndef SCANTAILOR_OUTPUT_FILLCOLORPROPERTY_H_
