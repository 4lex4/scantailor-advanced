// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "PictureShapeOptions.h"
#include <QDomDocument>

namespace output {
PictureShapeOptions::PictureShapeOptions()
    : m_pictureShape(FREE_SHAPE), m_sensitivity(100), m_higherSearchSensitivity(false) {}

PictureShapeOptions::PictureShapeOptions(const QDomElement& el)
    : m_pictureShape(parsePictureShape(el.attribute("pictureShape"))),
      m_sensitivity(el.attribute("sensitivity").toInt()),
      m_higherSearchSensitivity(el.attribute("higherSearchSensitivity") == "1") {}

QDomElement PictureShapeOptions::toXml(QDomDocument& doc, const QString& name) const {
  QDomElement el(doc.createElement(name));
  el.setAttribute("pictureShape", formatPictureShape(m_pictureShape));
  el.setAttribute("sensitivity", m_sensitivity);
  el.setAttribute("higherSearchSensitivity", m_higherSearchSensitivity ? "1" : "0");

  return el;
}

bool PictureShapeOptions::operator==(const PictureShapeOptions& other) const {
  return (m_pictureShape == other.m_pictureShape) && (m_sensitivity == other.m_sensitivity)
         && (m_higherSearchSensitivity == other.m_higherSearchSensitivity);
}

bool PictureShapeOptions::operator!=(const PictureShapeOptions& other) const {
  return !(*this == other);
}

PictureShape PictureShapeOptions::parsePictureShape(const QString& str) {
  if (str == "rectangular") {
    return RECTANGULAR_SHAPE;
  } else if (str == "off") {
    return OFF_SHAPE;
  } else {
    return FREE_SHAPE;
  }
}

QString PictureShapeOptions::formatPictureShape(PictureShape type) {
  QString str = "";
  switch (type) {
    case OFF_SHAPE:
      str = "off";
      break;
    case FREE_SHAPE:
      str = "free";
      break;
    case RECTANGULAR_SHAPE:
      str = "rectangular";
      break;
  }

  return str;
}

PictureShape PictureShapeOptions::getPictureShape() const {
  return m_pictureShape;
}

void PictureShapeOptions::setPictureShape(PictureShape pictureShape) {
  PictureShapeOptions::m_pictureShape = pictureShape;
}

int PictureShapeOptions::getSensitivity() const {
  return m_sensitivity;
}

void PictureShapeOptions::setSensitivity(int sensitivity) {
  PictureShapeOptions::m_sensitivity = sensitivity;
}

bool PictureShapeOptions::isHigherSearchSensitivity() const {
  return m_higherSearchSensitivity;
}

void PictureShapeOptions::setHigherSearchSensitivity(bool higherSearchSensitivity) {
  PictureShapeOptions::m_higherSearchSensitivity = higherSearchSensitivity;
}

}  // namespace output
