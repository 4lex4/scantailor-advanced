// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef XMLMARSHALLER_H_
#define XMLMARSHALLER_H_

#include <QDomDocument>
#include <QDomElement>
#include <QString>

class QSize;
class QSizeF;
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

  QDomElement pointF(const QPointF& p, const QString& name);

  QDomElement lineF(const QLineF& line, const QString& name);

  QDomElement rect(const QRect& rect, const QString& name);

  QDomElement rectF(const QRectF& rect, const QString& name);

  QDomElement polygonF(const QPolygonF& poly, const QString& name);

 private:
  QDomDocument m_doc;
};


#endif  // ifndef XMLMARSHALLER_H_
