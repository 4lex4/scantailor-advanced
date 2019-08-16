// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef ORTHOGONALROTATION_H_
#define ORTHOGONALROTATION_H_

class QString;
class QSize;
class QSizeF;
class QPointF;
class QTransform;
class QDomElement;
class QDomDocument;

class OrthogonalRotation {
 public:
  OrthogonalRotation();

  explicit OrthogonalRotation(const QDomElement& el);

  QDomElement toXml(QDomDocument& doc, const QString& name) const;

  bool operator==(const OrthogonalRotation& other) const;

  bool operator!=(const OrthogonalRotation& other) const;

  int toDegrees() const;

  void nextClockwiseDirection();

  void prevClockwiseDirection();

  QSize rotate(const QSize& dimensions) const;

  QSize unrotate(const QSize& dimensions) const;

  QSizeF rotate(const QSizeF& dimensions) const;

  QSizeF unrotate(const QSizeF& dimensions) const;

  QPointF rotate(const QPointF& point, double xmax, double ymax) const;

  QPointF unrotate(const QPointF& point, double xmax, double ymax) const;

  QTransform transform(const QSizeF& dimensions) const;

 private:
  int m_degrees;
};

#endif  // ifndef ORTHOGONALROTATION_H_
