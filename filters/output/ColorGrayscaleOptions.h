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

#ifndef OUTPUT_COLOR_GRAYSCALE_OPTIONS_H_
#define OUTPUT_COLOR_GRAYSCALE_OPTIONS_H_

#include <AutoManualMode.h>

class QString;
class QDomDocument;
class QDomElement;

namespace output
{

    class ColorGrayscaleOptions
    {
    public:
        ColorGrayscaleOptions();

        ColorGrayscaleOptions(QDomElement const& el);

        QDomElement toXml(QDomDocument& doc, QString const& name) const;

        bool whiteMargins() const
        { return m_whiteMargins; }

        void setWhiteMargins(bool val)
        { m_whiteMargins = val; }

        bool normalizeIllumination() const
        { return m_normalizeIllumination; }

        void setNormalizeIllumination(bool val)
        { m_normalizeIllumination = val; }

        bool cleanBackground() const
        { return m_cleanBackground; }

        void setCleanBackground(bool val)
        { m_cleanBackground = val; }

        AutoManualMode cleanMode() const
        { return m_cleanMode; }

        void setCleanMode(AutoManualMode val)
        { m_cleanMode = val; }

        int whitenAdjustment() const
        { return m_whitenAdjustment; }

        void setWhitenAdjustment(int val)
        { m_whitenAdjustment = val; }

        int brightnessAdjustment() const
        { return m_brightnessAdjustment; }

        void setBrightnessAdjustment(int val)
        { m_brightnessAdjustment = val; }

        int contrastAdjustment() const
        { return m_contrastAdjustment; }

        void setContrastAdjustment(int val)
        { m_contrastAdjustment = val; }

        bool operator==(ColorGrayscaleOptions const& other) const;

        bool operator!=(ColorGrayscaleOptions const& other) const;

    private:
        bool m_whiteMargins;
        bool m_normalizeIllumination;
        bool m_cleanBackground;
        AutoManualMode m_cleanMode;
        int m_whitenAdjustment;
        int m_brightnessAdjustment;
        int m_contrastAdjustment;
    };

}
#endif
