
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

#include "LineIntersectionScalar.h"

bool
lineIntersectionScalar(QLineF const& line1, QLineF const& line2, double& s1, double& s2)
{
    QPointF const p1(line1.p1());
    QPointF const p2(line2.p1());
    QPointF const v1(line1.p2() - line1.p1());
    QPointF const v2(line2.p2() - line2.p1());

    double const det_A = v2.x() * v1.y() - v1.x() * v2.y();
    if (fabs(det_A) < std::numeric_limits<double>::epsilon()) {
        return false;
    }

    double const r_det_A = 1.0 / det_A;
    QPointF const b(p2 - p1);
    s1 = (v2.x() * b.y() - b.x() * v2.y()) * r_det_A;
    s2 = (v1.x() * b.y() - b.x() * v1.y()) * r_det_A;

    return true;
}

bool
lineIntersectionScalar(QLineF const& line1, QLineF const& line2, double& s1)
{
    QPointF const p1(line1.p1());
    QPointF const p2(line2.p1());
    QPointF const v1(line1.p2() - line1.p1());
    QPointF const v2(line2.p2() - line2.p1());

    double const det_A = v2.x() * v1.y() - v1.x() * v2.y();
    if (fabs(det_A) < std::numeric_limits<double>::epsilon()) {
        return false;
    }

    QPointF const b(p2 - p1);
    s1 = (v2.x() * b.y() - b.x() * v2.y()) / det_A;

    return true;
}

