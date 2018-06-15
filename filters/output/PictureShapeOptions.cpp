
#include "PictureShapeOptions.h"
#include <QDomDocument>

namespace output {
PictureShapeOptions::PictureShapeOptions()
    : pictureShape(FREE_SHAPE), sensitivity(100), higherSearchSensitivity(false) {}

PictureShapeOptions::PictureShapeOptions(const QDomElement& el)
    : pictureShape(parsePictureShape(el.attribute("pictureShape"))),
      sensitivity(el.attribute("sensitivity").toInt()),
      higherSearchSensitivity(el.attribute("higherSearchSensitivity") == "1") {}

QDomElement PictureShapeOptions::toXml(QDomDocument& doc, const QString& name) const {
  QDomElement el(doc.createElement(name));
  el.setAttribute("pictureShape", formatPictureShape(pictureShape));
  el.setAttribute("sensitivity", sensitivity);
  el.setAttribute("higherSearchSensitivity", higherSearchSensitivity ? "1" : "0");

  return el;
}

bool PictureShapeOptions::operator==(const PictureShapeOptions& other) const {
  return (pictureShape == other.pictureShape) && (sensitivity == other.sensitivity)
         && (higherSearchSensitivity == other.higherSearchSensitivity);
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
  return pictureShape;
}

void PictureShapeOptions::setPictureShape(PictureShape pictureShape) {
  PictureShapeOptions::pictureShape = pictureShape;
}

int PictureShapeOptions::getSensitivity() const {
  return sensitivity;
}

void PictureShapeOptions::setSensitivity(int sensitivity) {
  PictureShapeOptions::sensitivity = sensitivity;
}

bool PictureShapeOptions::isHigherSearchSensitivity() const {
  return higherSearchSensitivity;
}

void PictureShapeOptions::setHigherSearchSensitivity(bool higherSearchSensitivity) {
  PictureShapeOptions::higherSearchSensitivity = higherSearchSensitivity;
}

}  // namespace output
