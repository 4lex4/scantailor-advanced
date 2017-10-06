/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2007-2009  Joseph Artsimovich <joseph_a@mail.ru>

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

#include "ColorCommonOptions.h"
#include <QDomDocument>

namespace output {
    ColorCommonOptions::ColorCommonOptions()
            : m_whiteMargins(true),
              m_normalizeIllumination(false) {
    }

    ColorCommonOptions::ColorCommonOptions(QDomElement const& el)
            : m_whiteMargins(el.attribute("whiteMargins") == "1"),
              m_normalizeIllumination(el.attribute("normalizeIlluminationColor") == "1") {
    }

    QDomElement ColorCommonOptions::toXml(QDomDocument& doc, QString const& name) const {
        QDomElement el(doc.createElement(name));
        el.setAttribute("whiteMargins", m_whiteMargins ? "1" : "0");
        el.setAttribute("normalizeIlluminationColor", m_normalizeIllumination ? "1" : "0");

        return el;
    }

    bool ColorCommonOptions::operator==(ColorCommonOptions const& other) const {
        return (m_normalizeIllumination == other.m_normalizeIllumination)
               && (m_whiteMargins == other.m_whiteMargins);
    }

    bool ColorCommonOptions::operator!=(ColorCommonOptions const& other) const {
        return !(*this == other);
    }
}  // namespace output