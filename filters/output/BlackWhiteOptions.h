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

#ifndef OUTPUT_BLACK_WHITE_OPTIONS_H_
#define OUTPUT_BLACK_WHITE_OPTIONS_H_

class QString;
class QDomDocument;
class QDomElement;

namespace output {
    class BlackWhiteOptions {
    public:
        enum BinarizationMethod {
            OTSU,
            SAUVOLA,
            WOLF
        };

        BlackWhiteOptions();

        explicit BlackWhiteOptions(QDomElement const& el);

        QDomElement toXml(QDomDocument& doc, QString const& name) const;

        bool operator==(BlackWhiteOptions const& other) const;

        bool operator!=(BlackWhiteOptions const& other) const;

        int thresholdAdjustment() const {
            return m_thresholdAdjustment;
        }

        void setThresholdAdjustment(int val) {
            m_thresholdAdjustment = val;
        }

        bool normalizeIllumination() const {
            return m_normalizeIllumination;
        }

        void setNormalizeIllumination(bool val) {
            m_normalizeIllumination = val;
        }

        bool isSavitzkyGolaySmoothingEnabled() const;

        void setSavitzkyGolaySmoothingEnabled(bool savitzkyGolaySmoothingEnabled);

        bool isMorphologicalSmoothingEnabled() const;

        void setMorphologicalSmoothingEnabled(bool morphologicalSmoothingEnabled);

        int getWindowSize() const;

        void setWindowSize(int windowSize);

        double getSauvolaCoef() const;

        void setSauvolaCoef(double sauvolaCoef);

        int getWolfLowerBound() const;

        void setWolfLowerBound(int wolfLowerBound);

        int getWolfUpperBound() const;

        void setWolfUpperBound(int wolfUpperBound);

        double getWolfCoef() const;

        void setWolfCoef(double wolfCoef);

        bool isWhiteOnBlackMode() const;

        void setWhiteOnBlackMode(bool whiteOnBlackMode);

        BinarizationMethod getBinarizationMethod() const;

        void setBinarizationMethod(BinarizationMethod binarizationMethod);

        bool isWhiteOnBlackAutoDetected() const;

        void setWhiteOnBlackAutoDetected(bool whiteOnBlackAutoDetected);

    private:
        static BinarizationMethod parseBinarizationMethod(const QString& str);

        static QString formatBinarizationMethod(BinarizationMethod type);

        int m_thresholdAdjustment;
        bool savitzkyGolaySmoothingEnabled;
        bool morphologicalSmoothingEnabled;
        bool m_normalizeIllumination;
        int windowSize;
        double sauvolaCoef;
        int wolfLowerBound;
        int wolfUpperBound;
        double wolfCoef;
        bool whiteOnBlackMode;
        BinarizationMethod binarizationMethod;

        bool whiteOnBlackAutoDetected;
    };
}
#endif  // ifndef OUTPUT_BLACK_WHITE_OPTIONS_H_
