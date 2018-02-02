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
            : m_cutMargins(true),
              m_normalizeIllumination(false),
              m_fillingColor(BACKGROUND),
              m_posterizeEnabled(false),
              m_posterizationLevel(4),
              m_forceBlackAndWhite(true) {
    }

    ColorCommonOptions::ColorCommonOptions(const QDomElement& el)
            : m_cutMargins(el.attribute("cutMargins") == "1"),
              m_normalizeIllumination(el.attribute("normalizeIlluminationColor") == "1"),
              m_fillingColor(parseFillingColor(el.attribute("fillingColor"))),
              m_posterizeEnabled(el.attribute("posterizeEnabled") == "1"),
              m_posterizationLevel(el.attribute("posterizationLevel").toInt()),
              m_forceBlackAndWhite(el.attribute("forceBlackAndWhite") == "1") {
    }

    QDomElement ColorCommonOptions::toXml(QDomDocument& doc, const QString& name) const {
        QDomElement el(doc.createElement(name));
        el.setAttribute("cutMargins", m_cutMargins ? "1" : "0");
        el.setAttribute("normalizeIlluminationColor", m_normalizeIllumination ? "1" : "0");
        el.setAttribute("fillingColor", formatFillingColor(m_fillingColor));
        el.setAttribute("posterizeEnabled", m_posterizeEnabled ? "1" : "0");
        el.setAttribute("posterizationLevel", m_posterizationLevel);
        el.setAttribute("forceBlackAndWhite", m_forceBlackAndWhite ? "1" : "0");

        return el;
    }

    bool ColorCommonOptions::operator==(const ColorCommonOptions& other) const {
        return (m_normalizeIllumination == other.m_normalizeIllumination)
               && (m_cutMargins == other.m_cutMargins)
               && (m_fillingColor == other.m_fillingColor)
               && (m_posterizeEnabled == other.m_posterizeEnabled)
               && (m_posterizationLevel == other.m_posterizationLevel)
               && (m_forceBlackAndWhite == other.m_forceBlackAndWhite);
    }

    bool ColorCommonOptions::operator!=(const ColorCommonOptions& other) const {
        return !(*this == other);
    }

    ColorCommonOptions::FillingColor ColorCommonOptions::getFillingColor() const {
        return m_fillingColor;
    }

    void ColorCommonOptions::setFillingColor(ColorCommonOptions::FillingColor fillingColor) {
        ColorCommonOptions::m_fillingColor = fillingColor;
    }

    ColorCommonOptions::FillingColor ColorCommonOptions::parseFillingColor(const QString& str) {
        if (str == "white") {
            return WHITE;
        } else {
            return BACKGROUND;
        }
    }

    QString ColorCommonOptions::formatFillingColor(ColorCommonOptions::FillingColor type) {
        QString str = "";
        switch (type) {
            case WHITE:
                str = "white";
                break;
            case BACKGROUND:
                str = "background";
                break;
        }

        return str;
    }

    void ColorCommonOptions::setCutMargins(bool val) {
        m_cutMargins = val;
    }

    bool ColorCommonOptions::cutMargins() const {
        return m_cutMargins;
    }

    bool ColorCommonOptions::normalizeIllumination() const {
        return m_normalizeIllumination;
    }

    void ColorCommonOptions::setNormalizeIllumination(bool val) {
        m_normalizeIllumination = val;
    }

    bool ColorCommonOptions::isPosterizeEnabled() const {
        return m_posterizeEnabled;
    }

    void ColorCommonOptions::setPosterizeEnabled(bool posterizeEnabled) {
        ColorCommonOptions::m_posterizeEnabled = posterizeEnabled;
    }

    int ColorCommonOptions::getPosterizationLevel() const {
        return m_posterizationLevel;
    }

    void ColorCommonOptions::setPosterizationLevel(int posterizationLevel) {
        ColorCommonOptions::m_posterizationLevel = posterizationLevel;
    }

    bool ColorCommonOptions::isForceBlackAndWhite() const {
        return m_forceBlackAndWhite;
    }

    void ColorCommonOptions::setForceBlackAndWhite(bool forceBlackAndWhite) {
        ColorCommonOptions::m_forceBlackAndWhite = forceBlackAndWhite;
    }
}  // namespace output