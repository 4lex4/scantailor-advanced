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

#include "Alignment.h"
#include <QDomDocument>

namespace page_layout {
    Alignment::Alignment()
            : m_vert(VCENTER),
              m_hor(HCENTER),
              m_tolerance(DEFAULT_TOLERANCE) {
    }

    Alignment::Alignment(Vertical vert, Horizontal hor)
            : m_vert(vert),
              m_hor(hor),
              m_tolerance(DEFAULT_TOLERANCE) {
    }

    Alignment::Alignment(const QDomElement& el) {
        const QString vert(el.attribute("vert"));
        const QString hor(el.attribute("hor"));
        m_isNull = el.attribute("null").toInt() != 0;
        m_tolerance = el.attribute("tolerance", QString::number(DEFAULT_TOLERANCE)).toDouble();

        if (vert == "top") {
            m_vert = TOP;
        } else if (vert == "bottom") {
            m_vert = BOTTOM;
        } else if (vert == "auto") {
            m_vert = VAUTO;
        } else if (vert == "original") {
            m_vert = VORIGINAL;
        } else {
            m_vert = VCENTER;
        }

        if (hor == "left") {
            m_hor = LEFT;
        } else if (hor == "right") {
            m_hor = RIGHT;
        } else if (hor == "auto") {
            m_hor = HAUTO;
        } else if (vert == "original") {
            m_hor = HORIGINAL;
        } else {
            m_hor = HCENTER;
        }
    }

    QDomElement Alignment::toXml(QDomDocument& doc, const QString& name) const {
        const char* vert = nullptr;
        switch (m_vert) {
            case TOP:
                vert = "top";
                break;
            case VCENTER:
                vert = "vcenter";
                break;
            case BOTTOM:
                vert = "bottom";
                break;
            case VAUTO:
                vert = "auto";
                break;
            case VORIGINAL:
                vert = "original";
                break;
        }

        const char* hor = nullptr;
        switch (m_hor) {
            case LEFT:
                hor = "left";
                break;
            case HCENTER:
                hor = "hcenter";
                break;
            case RIGHT:
                hor = "right";
                break;
            case HAUTO:
                hor = "auto";
                break;
            case HORIGINAL:
                hor = "original";
                break;
        }

        QDomElement el(doc.createElement(name));
        el.setAttribute("vert", QString::fromLatin1(vert));
        el.setAttribute("hor", QString::fromLatin1(hor));
        el.setAttribute("null", m_isNull ? 1 : 0);
        el.setAttribute("tolerance", QString::number(m_tolerance));

        return el;
    }

    bool Alignment::operator==(const Alignment& other) const {
        return (m_vert == other.m_vert)
               && (m_hor == other.m_hor)
               && (m_isNull == other.m_isNull);
    }

    bool Alignment::operator!=(const Alignment& other) const {
        return !(*this == other);
    }

    Alignment::Vertical Alignment::vertical() const {
        return m_vert;
    }

    void Alignment::setVertical(Alignment::Vertical vert) {
        m_vert = vert;
    }

    Alignment::Horizontal Alignment::horizontal() const {
        return m_hor;
    }

    void Alignment::setHorizontal(Alignment::Horizontal hor) {
        m_hor = hor;
    }

    bool Alignment::isNull() const {
        return m_isNull;
    }

    void Alignment::setNull(bool is_null) {
        m_isNull = is_null;
    }

    double Alignment::tolerance() const {
        return m_tolerance;
    }

    void Alignment::setTolerance(double t) {
        m_tolerance = t;
    }
}  // namespace page_layout
