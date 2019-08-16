// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "SerializableSpline.h"
#include <QTransform>
#include <boost/foreach.hpp>
#include "EditableSpline.h"
#include "XmlMarshaller.h"
#include "XmlUnmarshaller.h"

SerializableSpline::SerializableSpline(const EditableSpline& spline) {
  SplineVertex::Ptr vertex(spline.firstVertex());
  for (; vertex; vertex = vertex->next(SplineVertex::NO_LOOP)) {
    m_points.push_back(vertex->point());
  }
}

SerializableSpline::SerializableSpline(const QDomElement& el) {
  const QString point_str("point");

  QDomNode node(el.firstChild());
  for (; !node.isNull(); node = node.nextSibling()) {
    if (!node.isElement()) {
      continue;
    }
    if (node.nodeName() != point_str) {
      continue;
    }

    m_points.push_back(XmlUnmarshaller::pointF(node.toElement()));
  }
}

SerializableSpline::SerializableSpline(const QPolygonF& polygon) {
  for (int i = polygon.size() - 1; i >= 0; i--) {
    m_points.push_back(polygon[i]);
  }
}

QDomElement SerializableSpline::toXml(QDomDocument& doc, const QString& name) const {
  QDomElement el(doc.createElement(name));

  const QString point_str("point");
  XmlMarshaller marshaller(doc);
  for (const QPointF& pt : m_points) {
    el.appendChild(marshaller.pointF(pt, point_str));
  }

  return el;
}

SerializableSpline SerializableSpline::transformed(const QTransform& xform) const {
  SerializableSpline transformed(*this);

  for (QPointF& pt : transformed.m_points) {
    pt = xform.map(pt);
  }

  return transformed;
}

SerializableSpline SerializableSpline::transformed(const boost::function<QPointF(const QPointF&)>& xform) const {
  SerializableSpline transformed(*this);

  for (QPointF& pt : transformed.m_points) {
    pt = xform(pt);
  }

  return transformed;
}
