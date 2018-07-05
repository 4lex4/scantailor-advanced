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

#ifndef XMLMARSHALLER_H_
#define XMLMARSHALLER_H_

#include <QDomDocument>
#include <QDomElement>
#include <QString>

class QSize;
class QSizeF;
class Dpi;
class OrthogonalRotation;
class Margins;
class QPointF;
class QLineF;
class QPolygonF;
class QRect;
class QRectF;

class XmlMarshaller {
 public:
  explicit XmlMarshaller(const QDomDocument& doc) : m_doc(doc) {}

  QDomElement string(const QString& str, const QString& name);

  QDomElement size(const QSize& size, const QString& name);

  QDomElement sizeF(const QSizeF& size, const QString& name);

  QDomElement dpi(const Dpi& dpi, const QString& name);

  QDomElement rotation(const OrthogonalRotation& rotation, const QString& name);

  QDomElement pointF(const QPointF& p, const QString& name);

  QDomElement lineF(const QLineF& line, const QString& name);

  QDomElement rect(const QRect& rect, const QString& name);

  QDomElement rectF(const QRectF& rect, const QString& name);

  QDomElement polygonF(const QPolygonF& poly, const QString& name);

  QDomElement margins(const Margins& margins, const QString& name);

 private:
  QDomDocument m_doc;
};


#endif  // ifndef XMLMARSHALLER_H_
