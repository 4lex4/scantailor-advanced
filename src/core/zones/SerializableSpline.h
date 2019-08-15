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

#ifndef SERIALIZABLE_SPLINE_H_
#define SERIALIZABLE_SPLINE_H_

#include <QPointF>
#include <QPolygonF>
#include <QVector>
#include <boost/function.hpp>

class EditableSpline;
class QTransform;
class QDomDocument;
class QDomElement;
class QString;

class SerializableSpline {
 public:
  SerializableSpline(const EditableSpline& spline);

  explicit SerializableSpline(const QDomElement& el);

  explicit SerializableSpline(const QPolygonF& polygon);

  QDomElement toXml(QDomDocument& doc, const QString& name) const;

  SerializableSpline transformed(const QTransform& xform) const;

  SerializableSpline transformed(const boost::function<QPointF(const QPointF&)>& xform) const;

  QPolygonF toPolygon() const { return QPolygonF(m_points); }

 private:
  QVector<QPointF> m_points;
};


#endif  // ifndef SERIALIZABLE_SPLINE_H_
