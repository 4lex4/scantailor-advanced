/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2007-2008  Joseph Artsimovich <joseph_a@mail.ru>

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
