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

#include "DistortionModel.h"
#include "CylindricalSurfaceDewarper.h"
#include <QRectF>
#include <QTransform>
#include <QDomDocument>

namespace dewarping
{

    DistortionModel::DistortionModel()
    {
    }

    DistortionModel::DistortionModel(QDomElement const& el)
            : m_topCurve(el.namedItem("top-curve").toElement()),
              m_bottomCurve(el.namedItem("bottom-curve").toElement())
    {
    }

    QDomElement
    DistortionModel::toXml(QDomDocument& doc, QString const& name) const
    {
        if (!isValid()) {
            return QDomElement();
        }

        QDomElement el(doc.createElement(name));
        el.appendChild(m_topCurve.toXml(doc, "top-curve"));
        el.appendChild(m_bottomCurve.toXml(doc, "bottom-curve"));
        return el;
    }

    bool
    DistortionModel::isValid() const
    {
        if (!m_topCurve.isValid() || !m_bottomCurve.isValid()) {
            return false;
        }

        Vec2d const poly[4] = {
                m_topCurve.polyline().front(),
                m_topCurve.polyline().back(),
                m_bottomCurve.polyline().back(),
                m_bottomCurve.polyline().front()
        };

        double min_dot = NumericTraits<double>::max();
        double max_dot = NumericTraits<double>::min();

        for (int i = 0; i < 4; ++i) {
            Vec2d const cur(poly[i]);
            Vec2d const prev(poly[(i + 3) & 3]);
            Vec2d const next(poly[(i + 1) & 3]);

            Vec2d prev_normal(cur - prev);
            std::swap(prev_normal[0], prev_normal[1]);
            prev_normal[0] = -prev_normal[0];

            double const dot = prev_normal.dot(next - cur);
            if (dot < min_dot) {
                min_dot = dot;
            }
            if (dot > max_dot) {
                max_dot = dot;
            }
        }

        if (min_dot * max_dot <= 0) {
            return false;
        }

        if (fabs(min_dot) < 0.01 || fabs(max_dot) < 0.01) {
            return false;
        }

        return true;
    }

    bool
    DistortionModel::matches(DistortionModel const& other) const
    {
        bool const this_valid = isValid();
        bool const other_valid = other.isValid();
        if (!this_valid && !other_valid) {
            return true;
        }
        else if (this_valid != other_valid) {
            return false;
        }

        if (!m_topCurve.matches(other.m_topCurve)) {
            return false;
        }
        else if (!m_bottomCurve.matches(other.m_bottomCurve)) {
            return false;
        }

        return true;
    }

    QRectF
    DistortionModel::modelDomain(
            CylindricalSurfaceDewarper const& dewarper,
            QTransform const& to_output, QRectF const& output_content_rect) const
    {
        QRectF model_domain(boundingBox(to_output));

        double const vert_scale = 1.0 / dewarper.directrixArcLength();

        QPointF const scale_origin(output_content_rect.center());

        double const new_upper_part = (scale_origin.y() - model_domain.top()) * vert_scale;
        double const new_height = model_domain.height() * vert_scale;
        model_domain.setTop(scale_origin.y() - new_upper_part);
        model_domain.setHeight(new_height);

        return model_domain;
    }

    QRectF
    DistortionModel::boundingBox(QTransform const& transform) const
    {
        double top = NumericTraits<double>::max();
        double left = top;
        double bottom = NumericTraits<double>::min();
        double right = bottom;

        for (QPointF pt :  m_topCurve.polyline()) {
            pt = transform.map(pt);
            left = std::min<double>(left, pt.x());
            right = std::max<double>(right, pt.x());
            top = std::min<double>(top, pt.y());
            bottom = std::max<double>(bottom, pt.y());
        }

        for (QPointF pt :  m_bottomCurve.polyline()) {
            pt = transform.map(pt);
            left = std::min<double>(left, pt.x());
            right = std::max<double>(right, pt.x());
            top = std::min<double>(top, pt.y());
            bottom = std::max<double>(bottom, pt.y());
        }

        if (top > bottom || left > right) {
            return QRectF();
        }
        else {
            return QRectF(left, top, right - left, bottom - top);
        }
    }

} 