
#include "Guide.h"
#include "../../Utils.h"

namespace page_layout {
Guide::Guide(const Qt::Orientation orientation, const double position) : orientation(orientation), position(position) {}

Guide::Guide(const QLineF& line)
    : orientation(lineOrientation(line)), position((orientation == Qt::Horizontal) ? line.y1() : line.x1()) {}

Qt::Orientation Guide::getOrientation() const {
  return orientation;
}

double Guide::getPosition() const {
  return position;
}

void Guide::setPosition(double position) {
  Guide::position = position;
}

Qt::Orientation Guide::lineOrientation(const QLineF& line) {
  const double angle_cos = std::abs((line.p2().x() - line.p1().x()) / line.length());
  return (angle_cos > (1.0 / std::sqrt(2))) ? Qt::Horizontal : Qt::Vertical;
}

Guide::operator QLineF() const {
  if (orientation == Qt::Horizontal) {
    return QLineF(0, position, 1, position);
  } else {
    return QLineF(position, 0, position, 1);
  }
}

Guide::Guide(const QDomElement& el)
    : orientation(orientationFromString(el.attribute("orientation"))), position(el.attribute("position").toDouble()) {}

QDomElement Guide::toXml(QDomDocument& doc, const QString& name) const {
  QDomElement el = doc.createElement(name);

  el.setAttribute("orientation", orientationToString(orientation));
  el.setAttribute("position", Utils::doubleToString(position));

  return el;
}

QString Guide::orientationToString(const Qt::Orientation orientation) {
  if (orientation == Qt::Horizontal) {
    return "horizontal";
  } else {
    return "vertical";
  }
}

Qt::Orientation Guide::orientationFromString(const QString& str) {
  if (str == "vertical") {
    return Qt::Vertical;
  } else {
    return Qt::Horizontal;
  }
}

Guide::Guide() : orientation(Qt::Horizontal), position(0) {}
}  // namespace page_layout
