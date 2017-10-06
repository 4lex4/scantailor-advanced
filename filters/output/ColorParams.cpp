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

#include "ColorParams.h"
#include "CommandLine.h"

namespace output {
    ColorParams::ColorParams()
            : m_colorMode(DefaultColorMode()) {
    }

    ColorParams::ColorParams(QDomElement const& el)
            : m_colorMode(parseColorMode(el.attribute("colorMode"))),
              m_colorCommonOptions(el.namedItem("color-or-grayscale").toElement()),
              m_bwOptions(el.namedItem("bw").toElement()),
              m_splittingOptions(el.namedItem("splitting").toElement()) {
    }

    ColorParams::ColorMode ColorParams::DefaultColorMode() {
        return CommandLine::get().getDefaultColorMode();
    }

    QDomElement ColorParams::toXml(QDomDocument& doc, QString const& name) const {
        QDomElement el(doc.createElement(name));
        el.setAttribute("colorMode", formatColorMode(m_colorMode));
        el.appendChild(m_colorCommonOptions.toXml(doc, "color-or-grayscale"));
        el.appendChild(m_bwOptions.toXml(doc, "bw"));
        el.appendChild(m_splittingOptions.toXml(doc, "splitting"));

        return el;
    }

    ColorParams::ColorMode ColorParams::parseColorMode(QString const& str) {
        if (str == "bw") {
            return BLACK_AND_WHITE;
        } else if (str == "bitonal") {
            return BLACK_AND_WHITE;
        } else if (str == "colorOrGray") {
            return COLOR_GRAYSCALE;
        } else if (str == "mixed") {
            return MIXED;
        } else {
            return DefaultColorMode();
        }
    }

    QString ColorParams::formatColorMode(ColorParams::ColorMode const mode) {
        char const* str = "";
        switch (mode) {
            case BLACK_AND_WHITE:
                str = "bw";
                break;
            case COLOR_GRAYSCALE:
                str = "colorOrGray";
                break;
            case MIXED:
                str = "mixed";
                break;
        }

        return QString::fromLatin1(str);
    }
}  // namespace output