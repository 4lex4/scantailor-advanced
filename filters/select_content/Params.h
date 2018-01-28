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

#ifndef SELECT_CONTENT_PARAMS_H_
#define SELECT_CONTENT_PARAMS_H_

#include "Dependencies.h"
#include "AutoManualMode.h"
#include "Margins.h"
#include <QRectF>
#include <QSizeF>
#include <cmath>

class QDomDocument;
class QDomElement;
class QString;

namespace select_content {
    class Params {
    public:
        // Member-wise copying is OK.

        explicit Params(Dependencies const& deps);

        Params(QRectF const& content_rect,
               QSizeF const& size_mm,
               QRectF const& page_rect,
               Dependencies const& deps,
               AutoManualMode content_detection_mode,
               AutoManualMode page_detection_mode,
               bool contentDetect,
               bool pageDetect,
               bool fineTuning);

        explicit Params(QDomElement const& filter_el);

        QDomElement toXml(QDomDocument& doc, QString const& name) const;

        ~Params();

        QRectF const& contentRect() const;

        QRectF const& pageRect() const;

        QSizeF const& contentSizeMM() const;

        Dependencies const& dependencies() const;

        AutoManualMode contentDetectionMode() const;

        AutoManualMode pageDetectionMode() const;

        double deviation() const;

        void setDeviation(double d);

        void computeDeviation(double avg);

        bool isDeviant(double std, double max_dev);

        bool isContentDetectionEnabled() const;

        bool isPageDetectionEnabled() const;

        bool isFineTuningEnabled() const;

        void setContentDetectionMode(AutoManualMode const& mode);

        void setPageDetectionMode(AutoManualMode const& mode);

        void setContentRect(QRectF const& rect);

        void setPageRect(QRectF const& rect);

        void setContentSizeMM(QSizeF const& size);

        void setDependencies(Dependencies const& deps);

        void setContentDetect(bool detect);

        void setPageDetect(bool detect);

        void setFineTuneCorners(bool fine_tune);

    private:
        QRectF m_contentRect;
        QRectF m_pageRect;
        QSizeF m_contentSizeMM;
        Dependencies m_deps;
        AutoManualMode m_contentDetectionMode;
        AutoManualMode m_pageDetectionMode;
        bool m_contentDetectEnabled;
        bool m_pageDetectEnabled;
        bool m_fineTuneCorners;
        double m_deviation;
    };
}  // namespace select_content
#endif // ifndef SELECT_CONTENT_PARAMS_H_
