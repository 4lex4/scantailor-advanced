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

#ifndef XMLUNMARSHALLER_H_
#define XMLUNMARSHALLER_H_

class QString;
class QDomElement;
class QSize;
class QSizeF;
class Dpi;
class OrthogonalRotation;
class Margins;
class QPointF;
class QLineF;
class QRect;
class QRectF;
class QPolygonF;

class XmlUnmarshaller {
 public:
  static QString string(const QDomElement& el);

  static QSize size(const QDomElement& el);

  static QSizeF sizeF(const QDomElement& el);

  static Dpi dpi(const QDomElement& el);

  static OrthogonalRotation rotation(const QDomElement& el);

  static Margins margins(const QDomElement& el);

  static QPointF pointF(const QDomElement& el);

  static QLineF lineF(const QDomElement& el);

  static QRect rect(const QDomElement& el);

  static QRectF rectF(const QDomElement& el);

  static QPolygonF polygonF(const QDomElement& el);
};


#endif  // ifndef XMLUNMARSHALLER_H_
