// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

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
