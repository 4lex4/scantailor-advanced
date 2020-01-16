// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "OrthogonalRotation.h"

#include <QDomDocument>
#include <QDomElement>
#include <QPointF>
#include <QSize>
#include <QString>
#include <QTransform>
#include <cassert>

OrthogonalRotation::OrthogonalRotation() : m_degrees(0) {}

OrthogonalRotation::OrthogonalRotation(const QDomElement& el) : m_degrees(el.attribute("degrees").toInt()) {}

int OrthogonalRotation::toDegrees() const {
  return m_degrees;
}

void OrthogonalRotation::nextClockwiseDirection() {
  m_degrees += 90;
  if (m_degrees == 360) {
    m_degrees = 0;
  }
}

void OrthogonalRotation::prevClockwiseDirection() {
  m_degrees -= 90;
  if (m_degrees == -90) {
    m_degrees = 270;
  }
}

QSize OrthogonalRotation::rotate(const QSize& dimensions) const {
  if ((m_degrees == 90) || (m_degrees == 270)) {
    return QSize(dimensions.height(), dimensions.width());
  } else {
    return dimensions;
  }
}

QSize OrthogonalRotation::unrotate(const QSize& dimensions) const {
  return rotate(dimensions);
}

QSizeF OrthogonalRotation::rotate(const QSizeF& dimensions) const {
  if ((m_degrees == 90) || (m_degrees == 270)) {
    return QSizeF(dimensions.height(), dimensions.width());
  } else {
    return dimensions;
  }
}

QSizeF OrthogonalRotation::unrotate(const QSizeF& dimensions) const {
  return rotate(dimensions);
}

QPointF OrthogonalRotation::rotate(const QPointF& point, const double xmax, const double ymax) const {
  QPointF rotated;

  switch (m_degrees) {
    case 0:
      rotated = point;
      break;
    case 90:
      rotated.setX(ymax - point.y());
      rotated.setY(point.x());
      break;
    case 180:
      rotated.setX(xmax - point.x());
      rotated.setY(ymax - point.y());
      break;
    case 270:
      rotated.setX(point.y());
      rotated.setY(xmax - point.x());
      break;
    default:
      assert(!"Unreachable");
  }
  return rotated;
}

QPointF OrthogonalRotation::unrotate(const QPointF& point, const double xmax, const double ymax) const {
  QPointF unrotated;

  switch (m_degrees) {
    case 0:
      unrotated = point;
      break;
    case 90:
      unrotated.setX(point.y());
      unrotated.setY(xmax - point.x());
      break;
    case 180:
      unrotated.setX(xmax - point.x());
      unrotated.setY(ymax - point.y());
      break;
    case 270:
      unrotated.setX(ymax - point.y());
      unrotated.setY(point.x());
      break;
    default:
      assert(!"Unreachable");
  }
  return unrotated;
}

QTransform OrthogonalRotation::transform(const QSizeF& dimensions) const {
  QTransform t;

  switch (m_degrees) {
    case 0:
      return t;
    case 90:
      t.translate(dimensions.height(), 0);
      break;
    case 180:
      t.translate(dimensions.width(), dimensions.height());
      break;
    case 270:
      t.translate(0, dimensions.width());
      break;
    default:
      assert(!"Unreachable");
  }

  t.rotate(m_degrees);
  return t;
}

QDomElement OrthogonalRotation::toXml(QDomDocument& doc, const QString& name) const {
  QDomElement el(doc.createElement(name));
  el.setAttribute("degrees", m_degrees);
  return el;
}

bool OrthogonalRotation::operator==(const OrthogonalRotation& other) const {
  return m_degrees == other.m_degrees;
}

bool OrthogonalRotation::operator!=(const OrthogonalRotation& other) const {
  return !(*this == other);
}
