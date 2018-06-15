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

#ifndef DEWARPING_CURVE_H_
#define DEWARPING_CURVE_H_

#include <QPointF>
#include <vector>
#include "XSpline.h"

class QDomDocument;
class QDomElement;
class QString;

namespace dewarping {
class Curve {
 public:
  Curve();

  explicit Curve(const std::vector<QPointF>& polyline);

  explicit Curve(const XSpline& xspline);

  explicit Curve(const QDomElement& el);

  QDomElement toXml(QDomDocument& doc, const QString& name) const;

  bool isValid() const;

  bool matches(const Curve& other) const;

  const XSpline& xspline() const { return m_xspline; }

  const std::vector<QPointF>& polyline() const { return m_polyline; }

  static bool splineHasLoops(const XSpline& spline);

 private:
  struct CloseEnough;

  static std::vector<QPointF> deserializePolyline(const QDomElement& el);

  static QDomElement serializePolyline(const std::vector<QPointF>& polyline, QDomDocument& doc, const QString& name);

  static XSpline deserializeXSpline(const QDomElement& el);

  static QDomElement serializeXSpline(const XSpline& xspline, QDomDocument& doc, const QString& name);

  static bool approxPolylineMatch(const std::vector<QPointF>& polyline1, const std::vector<QPointF>& polyline2);

  XSpline m_xspline;
  std::vector<QPointF> m_polyline;
};
}  // namespace dewarping
#endif  // ifndef DEWARPING_CURVE_H_
