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

namespace output {
    class ColorCommonOptions {
    public:
        enum FillingColor {
            BACKGROUND,
            WHITE
        };

        ColorCommonOptions();

        explicit ColorCommonOptions(QDomElement const& el);

        QDomElement toXml(QDomDocument& doc, QString const& name) const;

        bool cutMargins() const;

        void setCutMargins(bool val);

        bool normalizeIllumination() const;

        void setNormalizeIllumination(bool val);

        FillingColor getFillingColor() const;

        void setFillingColor(FillingColor fillingColor);

        bool operator==(ColorCommonOptions const& other) const;

        bool operator!=(ColorCommonOptions const& other) const;

    private:
        static FillingColor parseFillingColor(const QString& str);

        static QString formatFillingColor(FillingColor type);


        bool m_cutMargins;
        bool m_normalizeIllumination;
        FillingColor m_fillingColor;
    };
}  // namespace output
#endif  // ifndef OUTPUT_COLOR_GRAYSCALE_OPTIONS_H_
