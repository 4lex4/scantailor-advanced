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
              binarizationMethod(OTSU),
              colorSegmentationEnabled(false),
              segmentationNoiseReduction(68) {
    }

    BlackWhiteOptions::BlackWhiteOptions(const QDomElement& el)
            : m_thresholdAdjustment(el.attribute("thresholdAdj").toInt()),
              savitzkyGolaySmoothingEnabled(el.attribute("savitzkyGolaySmoothing") == "1"),
              morphologicalSmoothingEnabled(el.attribute("morphologicalSmoothing") == "1"),
              m_normalizeIllumination(el.attribute("normalizeIlluminationBW") == "1"),
              windowSize(el.attribute("windowSize").toInt()),
              sauvolaCoef(el.attribute("sauvolaCoef").toDouble()),
              wolfLowerBound(el.attribute("wolfLowerBound").toInt()),
              wolfUpperBound(el.attribute("wolfUpperBound").toInt()),
              wolfCoef(el.attribute("wolfCoef").toDouble()),
              binarizationMethod(parseBinarizationMethod(el.attribute("binarizationMethod"))),
              colorSegmentationEnabled(el.attribute("colorSegmentationEnabled") == "1"),
              segmentationNoiseReduction(el.attribute("segmentationNoiseReduction").toInt()) {
    }

    QDomElement BlackWhiteOptions::toXml(QDomDocument& doc, const QString& name) const {
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
        el.setAttribute("binarizationMethod", formatBinarizationMethod(binarizationMethod));
        el.setAttribute("colorSegmentationEnabled", colorSegmentationEnabled ? "1" : "0");
        el.setAttribute("segmentationNoiseReduction", segmentationNoiseReduction);

        return el;
    }

    bool BlackWhiteOptions::operator==(const BlackWhiteOptions& other) const {
        return (m_thresholdAdjustment == other.m_thresholdAdjustment)
               && (savitzkyGolaySmoothingEnabled == other.savitzkyGolaySmoothingEnabled)
               && (morphologicalSmoothingEnabled == other.morphologicalSmoothingEnabled)
               && (m_normalizeIllumination == other.m_normalizeIllumination)
               && (windowSize == other.windowSize)
               && (sauvolaCoef == other.sauvolaCoef)
               && (wolfLowerBound == other.wolfLowerBound)
               && (wolfUpperBound == other.wolfUpperBound)
               && (wolfCoef == other.wolfCoef)
               && (binarizationMethod == other.binarizationMethod)
               && (colorSegmentationEnabled == other.colorSegmentationEnabled)
               && (segmentationNoiseReduction == other.segmentationNoiseReduction);
    }

    bool BlackWhiteOptions::operator!=(const BlackWhiteOptions& other) const {
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

    BinarizationMethod BlackWhiteOptions::getBinarizationMethod() const {
        return binarizationMethod;
    }

    void BlackWhiteOptions::setBinarizationMethod(BinarizationMethod binarizationMethod) {
        BlackWhiteOptions::binarizationMethod = binarizationMethod;
    }

    BinarizationMethod BlackWhiteOptions::parseBinarizationMethod(const QString& str) {
        if (str == "wolf") {
            return WOLF;
        } else if (str == "sauvola") {
            return SAUVOLA;
        } else {
            return OTSU;
        }
    }

    QString BlackWhiteOptions::formatBinarizationMethod(BinarizationMethod type) {
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

    int BlackWhiteOptions::thresholdAdjustment() const {
        return m_thresholdAdjustment;
    }

    void BlackWhiteOptions::setThresholdAdjustment(int val) {
        m_thresholdAdjustment = val;
    }

    bool BlackWhiteOptions::normalizeIllumination() const {
        return m_normalizeIllumination;
    }

    void BlackWhiteOptions::setNormalizeIllumination(bool val) {
        m_normalizeIllumination = val;
    }

    bool BlackWhiteOptions::isColorSegmentationEnabled() const {
        return colorSegmentationEnabled;
    }

    void BlackWhiteOptions::setColorSegmentationEnabled(bool colorSegmentationEnabled) {
        BlackWhiteOptions::colorSegmentationEnabled = colorSegmentationEnabled;
    }

    int BlackWhiteOptions::getSegmentationNoiseReduction() const {
        return segmentationNoiseReduction;
    }

    void BlackWhiteOptions::setSegmentationNoiseReduction(int segmentationNoiseReduction) {
        BlackWhiteOptions::segmentationNoiseReduction = segmentationNoiseReduction;
    }

}  // namespace output