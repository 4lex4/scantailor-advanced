// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef XMLUNMARSHALLER_H_
#define XMLUNMARSHALLER_H_

class QString;
class QDomElement;
class QSize;
class QSizeF;
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

  static QPointF pointF(const QDomElement& el);

  static QLineF lineF(const QDomElement& el);

  static QRect rect(const QDomElement& el);

  static QRectF rectF(const QDomElement& el);

  static QPolygonF polygonF(const QDomElement& el);
};


#endif  // ifndef XMLUNMARSHALLER_H_
