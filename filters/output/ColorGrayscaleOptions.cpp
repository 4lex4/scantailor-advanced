
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

#include "ColorGrayscaleOptions.h"
#include <QDomDocument>

namespace output
{
    ColorGrayscaleOptions::ColorGrayscaleOptions()
        : m_whiteMargins(false),
          m_normalizeIllumination(false),
          m_cleanBackground(false),
          m_cleanMode(MODE_AUTO),
          m_whitenAdjustment(0),
          m_brightnessAdjustment(0),
          m_contrastAdjustment(0)
    { }

    ColorGrayscaleOptions::ColorGrayscaleOptions(QDomElement const& el)
        : m_whiteMargins(el.attribute("whiteMargins") == "1"),
          m_normalizeIllumination(el.attribute("normalizeIllumination") == "1"),
          m_cleanBackground(el.attribute("cleanBackground") == "1"),
          m_cleanMode(el.attribute("cleanMode") == "manual" ? MODE_MANUAL : MODE_AUTO),
          m_whitenAdjustment(el.attribute("whitenAdj").toInt()),
          m_brightnessAdjustment(el.attribute("brightnessAdj").toInt()),
          m_contrastAdjustment(el.attribute("contrastAdj").toInt())
    { }

    QDomElement
    ColorGrayscaleOptions::toXml(QDomDocument& doc, QString const& name) const
    {
        QDomElement el(doc.createElement(name));
        el.setAttribute("whiteMargins", m_whiteMargins ? "1" : "0");
        el.setAttribute("normalizeIllumination", m_normalizeIllumination ? "1" : "0");
        el.setAttribute("cleanBackground", m_cleanBackground ? "1" : "0");
        el.setAttribute("cleanMode", (m_cleanMode == MODE_AUTO) ? "auto" : "manual");
        el.setAttribute("whitenAdj", m_whitenAdjustment);
        el.setAttribute("brightnessAdj", m_brightnessAdjustment);
        el.setAttribute("contrastAdj", m_contrastAdjustment);

        return el;
    }

    bool
    ColorGrayscaleOptions::operator==(ColorGrayscaleOptions const& other) const
    {
        if (m_whiteMargins != other.m_whiteMargins) {
            return false;
        }

        if (m_normalizeIllumination != other.m_normalizeIllumination) {
            return false;
        }

        if (m_cleanBackground != other.m_cleanBackground) {
            return false;
        }

        if (m_cleanMode != other.m_cleanMode) {
            return false;
        }

        if (m_whitenAdjustment != other.m_whitenAdjustment) {
            return false;
        }

        if (m_brightnessAdjustment != other.m_brightnessAdjustment) {
            return false;
        }

        if (m_contrastAdjustment != other.m_contrastAdjustment) {
            return false;
        }

        return true;
    }  // ==

    bool
    ColorGrayscaleOptions::operator!=(ColorGrayscaleOptions const& other) const
    {
        return !(*this == other);
    }
}  // namespace output