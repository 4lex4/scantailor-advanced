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

#ifndef OUTPUT_DEWARPING_MODE_H_
#define OUTPUT_DEWARPING_MODE_H_

#include <QString>
#include <QtXml/QDomElement>

namespace output {
    class DewarpingOptions {
    public:
        enum Mode {
            OFF,
            AUTO,
            MANUAL,
            MARGINAL
        };

        explicit DewarpingOptions(Mode mode = OFF, bool needPostDeskew = true);

        explicit DewarpingOptions(QDomElement const& el);

        QDomElement toXml(QDomDocument& doc, QString const& name) const;

        bool operator==(DewarpingOptions const& other) const;

        bool operator!=(DewarpingOptions const& other) const;

        DewarpingOptions::Mode mode() const;

        void setMode(DewarpingOptions::Mode m_mode);

        bool needPostDeskew() const;

        void setPostDeskew(bool postDeskew);

        static DewarpingOptions::Mode parseDewarpingMode(QString const& str);

        static QString formatDewarpingMode(DewarpingOptions::Mode mode);

    private:
        DewarpingOptions::Mode m_mode;
        bool postDeskew;
    };
}
#endif
