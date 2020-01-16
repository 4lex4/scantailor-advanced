// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "Guide.h"

#include <foundation/Utils.h>

using namespace foundation;

namespace page_layout {
Guide::Guide(const Qt::Orientation orientation, const double position)
    : m_orientation(orientation), m_position(position) {}

Guide::Guide(const QLineF& line)
    : m_orientation(lineOrientation(line)), m_position((m_orientation == Qt::Horizontal) ? line.y1() : line.x1()) {}

Qt::Orientation Guide::getOrientation() const {
  return m_orientation;
}

double Guide::getPosition() const {
  return m_position;
}

void Guide::setPosition(double position) {
  Guide::m_position = position;
}

Qt::Orientation Guide::lineOrientation(const QLineF& line) {
  const double angleCos = std::abs((line.p2().x() - line.p1().x()) / line.length());
  return (angleCos > (1.0 / std::sqrt(2))) ? Qt::Horizontal : Qt::Vertical;
}

Guide::operator QLineF() const {
  if (m_orientation == Qt::Horizontal) {
    return QLineF(0, m_position, 1, m_position);
  } else {
    return QLineF(m_position, 0, m_position, 1);
  }
}

Guide::Guide(const QDomElement& el)
    : m_orientation(orientationFromString(el.attribute("orientation"))),
      m_position(el.attribute("position").toDouble()) {}

QDomElement Guide::toXml(QDomDocument& doc, const QString& name) const {
  QDomElement el = doc.createElement(name);

  el.setAttribute("orientation", orientationToString(m_orientation));
  el.setAttribute("position", Utils::doubleToString(m_position));
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

Guide::Guide() : m_orientation(Qt::Horizontal), m_position(0) {}
}  // namespace page_layout
