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

#include "BlackWhiteOptions.h"
#include <QDomDocument>

namespace output {
    BlackWhiteOptions::BlackWhiteOptions()
            : m_thresholdAdjustment(0),
              savitzkyGolaySmoothingEnabled(true),
              morphologicalSmoothingEnabled(true),
              m_normalizeIllumination(true),
              windowSize(51),
              sauvolaCoef(0.34),
              wolfLowerBound(1),
              wolfUpperBound(254),
              wolfCoef(0.3),
              whiteOnBlackMode(false),
              binarizationMethod(OTSU),
              whiteOnBlackAutoDetected(false) {
    }

    BlackWhiteOptions::BlackWhiteOptions(QDomElement const& el)
            : m_thresholdAdjustment(el.attribute("thresholdAdj").toInt()),
              savitzkyGolaySmoothingEnabled(el.attribute("savitzkyGolaySmoothing") == "1"),
              morphologicalSmoothingEnabled(el.attribute("morphologicalSmoothing") == "1"),
              m_normalizeIllumination(el.attribute("normalizeIlluminationBW") == "1"),
              windowSize(el.attribute("windowSize").toInt()),
              sauvolaCoef(el.attribute("sauvolaCoef").toDouble()),
              wolfLowerBound(el.attribute("wolfLowerBound").toInt()),
              wolfUpperBound(el.attribute("wolfUpperBound").toInt()),
              wolfCoef(el.attribute("wolfCoef").toDouble()),
              whiteOnBlackMode(el.attribute("whiteOnBlackMode") == "1"),
              binarizationMethod(parseBinarizationMethod(el.attribute("binarizationMethod"))),
              whiteOnBlackAutoDetected(el.attribute("whiteOnBlackAutoDetected") == "1") {
    }

    QDomElement BlackWhiteOptions::toXml(QDomDocument& doc, QString const& name) const {
        QDomElement el(doc.createElement(name));
        el.setAttribute("thresholdAdj", m_thresholdAdjustment);
        el.setAttribute("savitzkyGolaySmoothing", savitzkyGolaySmoothingEnabled ? "1" : "0");
        el.setAttribute("morphologicalSmoothing", morphologicalSmoothingEnabled ? "1" : "0");
        el.setAttribute("normalizeIlluminationBW", m_normalizeIllumination ? "1" : "0");
        el.setAttribute("windowSize", windowSize);
        el.setAttribute("sauvolaCoef", sauvolaCoef);
        el.setAttribute("wolfLowerBound", wolfLowerBound);
        el.setAttribute("wolfUpperBound", wolfUpperBound);
        el.setAttribute("wolfCoef", wolfCoef);
        el.setAttribute("whiteOnBlackMode", whiteOnBlackMode ? "1" : "0");
        el.setAttribute("binarizationMethod", formatBinarizationMethod(binarizationMethod));
        el.setAttribute("whiteOnBlackAutoDetected", whiteOnBlackAutoDetected ? "1" : "0");

        return el;
    }

    bool BlackWhiteOptions::operator==(BlackWhiteOptions const& other) const {
        return (m_thresholdAdjustment == other.m_thresholdAdjustment)
               && (savitzkyGolaySmoothingEnabled == other.savitzkyGolaySmoothingEnabled)
               && (morphologicalSmoothingEnabled == other.morphologicalSmoothingEnabled)
               && (m_normalizeIllumination == other.m_normalizeIllumination)
               && (windowSize == other.windowSize)
               && (sauvolaCoef == other.sauvolaCoef)
               && (wolfLowerBound == other.wolfLowerBound)
               && (wolfUpperBound == other.wolfUpperBound)
               && (wolfCoef == other.wolfCoef)
               && (whiteOnBlackMode == other.whiteOnBlackMode)
               && (binarizationMethod == other.binarizationMethod);
    }

    bool BlackWhiteOptions::operator!=(BlackWhiteOptions const& other) const {
        return !(*this == other);
    }

    bool BlackWhiteOptions::isSavitzkyGolaySmoothingEnabled() const {
        return savitzkyGolaySmoothingEnabled;
    }

    void BlackWhiteOptions::setSavitzkyGolaySmoothingEnabled(bool savitzkyGolaySmoothingEnabled) {
        BlackWhiteOptions::savitzkyGolaySmoothingEnabled = savitzkyGolaySmoothingEnabled;
    }

    bool BlackWhiteOptions::isMorphologicalSmoothingEnabled() const {
        return morphologicalSmoothingEnabled;
    }

    void BlackWhiteOptions::setMorphologicalSmoothingEnabled(bool morphologicalSmoothingEnabled) {
        BlackWhiteOptions::morphologicalSmoothingEnabled = morphologicalSmoothingEnabled;
    }

    int BlackWhiteOptions::getWindowSize() const {
        return windowSize;
    }

    void BlackWhiteOptions::setWindowSize(int windowSize) {
        BlackWhiteOptions::windowSize = windowSize;
    }

    double BlackWhiteOptions::getSauvolaCoef() const {
        return sauvolaCoef;
    }

    void BlackWhiteOptions::setSauvolaCoef(double sauvolaCoef) {
        BlackWhiteOptions::sauvolaCoef = sauvolaCoef;
    }

    int BlackWhiteOptions::getWolfLowerBound() const {
        return wolfLowerBound;
    }

    void BlackWhiteOptions::setWolfLowerBound(int wolfLowerBound) {
        BlackWhiteOptions::wolfLowerBound = wolfLowerBound;
    }

    int BlackWhiteOptions::getWolfUpperBound() const {
        return wolfUpperBound;
    }

    void BlackWhiteOptions::setWolfUpperBound(int wolfUpperBound) {
        BlackWhiteOptions::wolfUpperBound = wolfUpperBound;
    }

    double BlackWhiteOptions::getWolfCoef() const {
        return wolfCoef;
    }

    void BlackWhiteOptions::setWolfCoef(double wolfCoef) {
        BlackWhiteOptions::wolfCoef = wolfCoef;
    }

    bool BlackWhiteOptions::isWhiteOnBlackMode() const {
        return whiteOnBlackMode;
    }

    void BlackWhiteOptions::setWhiteOnBlackMode(bool whiteOnBlackMode) {
        BlackWhiteOptions::whiteOnBlackMode = whiteOnBlackMode;
    }

    BlackWhiteOptions::BinarizationMethod BlackWhiteOptions::getBinarizationMethod() const {
        return binarizationMethod;
    }

    void BlackWhiteOptions::setBinarizationMethod(BlackWhiteOptions::BinarizationMethod binarizationMethod) {
        BlackWhiteOptions::binarizationMethod = binarizationMethod;
    }

    BlackWhiteOptions::BinarizationMethod BlackWhiteOptions::parseBinarizationMethod(const QString& str) {
        if (str == "wolf") {
            return WOLF;
        } else if (str == "sauvola") {
            return SAUVOLA;
        } else {
            return OTSU;
        }
    }

    QString BlackWhiteOptions::formatBinarizationMethod(BlackWhiteOptions::BinarizationMethod type) {
        QString str = "";
        switch (type) {
            case OTSU:
                str = "otsu";
                break;
            case SAUVOLA:
                str = "sauvola";
                break;
            case WOLF:
                str = "wolf";
                break;
        }

        return str;
    }

    bool BlackWhiteOptions::isWhiteOnBlackAutoDetected() const {
        return whiteOnBlackAutoDetected;
    }

    void BlackWhiteOptions::setWhiteOnBlackAutoDetected(bool whiteOnBlackAutoDetected) {
        BlackWhiteOptions::whiteOnBlackAutoDetected = whiteOnBlackAutoDetected;
    }
}  // namespace output