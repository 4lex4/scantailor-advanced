// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "PictureLayerProperty.h"

#include <QDomDocument>

#include "PropertyFactory.h"

namespace output {
const char PictureLayerProperty::m_propertyName[] = "PictureZoneProperty";

PictureLayerProperty::PictureLayerProperty(const QDomElement& el) : m_layer(layerFromString(el.attribute("layer"))) {}

void PictureLayerProperty::registerIn(PropertyFactory& factory) {
  factory.registerProperty(m_propertyName, &PictureLayerProperty::construct);
}

std::shared_ptr<Property> PictureLayerProperty::clone() const {
  return std::make_shared<PictureLayerProperty>(*this);
}

QDomElement PictureLayerProperty::toXml(QDomDocument& doc, const QString& name) const {
  QDomElement el(doc.createElement(name));
  el.setAttribute("type", m_propertyName);
  el.setAttribute("layer", layerToString(m_layer));
  return el;
}

std::shared_ptr<Property> PictureLayerProperty::construct(const QDomElement& el) {
  return std::make_shared<PictureLayerProperty>(el);
}

PictureLayerProperty::Layer PictureLayerProperty::layerFromString(const QString& str) {
  if (str == "eraser1") {
    return ZONEERASER1;
  } else if (str == "painter2") {
    return ZONEPAINTER2;
  } else if (str == "eraser3") {
    return ZONEERASER3;
  } else if (str == "foreground") {
    return ZONEFG;
  } else if (str == "background") {
    return ZONEBG;
  } else {
    return ZONENOOP;
  }
}

QString PictureLayerProperty::layerToString(Layer layer) {
  QString str;

  switch (layer) {
    case ZONEERASER1:
      str = "eraser1";
      break;
    case ZONEPAINTER2:
      str = "painter2";
      break;
    case ZONEERASER3:
      str = "eraser3";
      break;
    case ZONEFG:
      str = "foreground";
      break;
    case ZONEBG:
      str = "background";
      break;
    default:
      str = "";
      break;
  }
  return str;
}

PictureLayerProperty::PictureLayerProperty(PictureLayerProperty::Layer layer) : m_layer(layer) {}
}  // namespace output
