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

#include "XmlUnmarshaller.h"
#include <QDomElement>
#include <QLineF>
#include <QPointF>
#include <QPolygonF>
#include <QRect>
#include <QString>
#include "Dpi.h"
#include "Margins.h"
#include "OrthogonalRotation.h"

QString XmlUnmarshaller::string(const QDomElement& el) {
  return el.text();  // FIXME: this needs unescaping, but Qt doesn't provide such functionality
}

QSize XmlUnmarshaller::size(const QDomElement& el) {
  const int width = el.attribute("width").toInt();
  const int height = el.attribute("height").toInt();

  return QSize(width, height);
}

QSizeF XmlUnmarshaller::sizeF(const QDomElement& el) {
  const double width = el.attribute("width").toDouble();
  const double height = el.attribute("height").toDouble();

  return QSizeF(width, height);
}

Dpi XmlUnmarshaller::dpi(const QDomElement& el) {
  const int hor = el.attribute("horizontal").toInt();
  const int ver = el.attribute("vertical").toInt();

  return Dpi(hor, ver);
}

OrthogonalRotation XmlUnmarshaller::rotation(const QDomElement& el) {
  const int degrees = el.attribute("degrees").toInt();
  OrthogonalRotation rotation;
  for (int i = 0; i < 4; ++i) {
    if (rotation.toDegrees() == degrees) {
      break;
    }
    rotation.nextClockwiseDirection();
  }

  return rotation;
}

Margins XmlUnmarshaller::margins(const QDomElement& el) {
  Margins margins;
  margins.setLeft(el.attribute("left").toDouble());
  margins.setRight(el.attribute("right").toDouble());
  margins.setTop(el.attribute("top").toDouble());
  margins.setBottom(el.attribute("bottom").toDouble());

  return margins;
}

QPointF XmlUnmarshaller::pointF(const QDomElement& el) {
  const double x = el.attribute("x").toDouble();
  const double y = el.attribute("y").toDouble();

  return QPointF(x, y);
}

QLineF XmlUnmarshaller::lineF(const QDomElement& el) {
  const QPointF p1(pointF(el.namedItem("p1").toElement()));
  const QPointF p2(pointF(el.namedItem("p2").toElement()));

  return QLineF(p1, p2);
}

QRect XmlUnmarshaller::rect(const QDomElement& el) {
  const int x = el.attribute("x").toInt();
  const int y = el.attribute("y").toInt();
  const int width = el.attribute("width").toInt();
  const int height = el.attribute("height").toInt();

  return QRect(x, y, width, height);
}

QRectF XmlUnmarshaller::rectF(const QDomElement& el) {
  const double x = el.attribute("x").toDouble();
  const double y = el.attribute("y").toDouble();
  const double width = el.attribute("width").toDouble();
  const double height = el.attribute("height").toDouble();

  return QRectF(x, y, width, height);
}

QPolygonF XmlUnmarshaller::polygonF(const QDomElement& el) {
  QPolygonF poly;

  const QString point_tag_name("point");
  QDomNode node(el.firstChild());
  for (; !node.isNull(); node = node.nextSibling()) {
    if (!node.isElement()) {
      continue;
    }
    if (node.nodeName() != point_tag_name) {
      continue;
    }
    poly.push_back(pointF(node.toElement()));
  }

  return poly;
}
