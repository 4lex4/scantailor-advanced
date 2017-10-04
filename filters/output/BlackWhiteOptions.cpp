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
              m_normalizeIllumination(true) {
    }

    BlackWhiteOptions::BlackWhiteOptions(QDomElement const& el)
            : m_thresholdAdjustment(el.attribute("thresholdAdj").toInt()),
              savitzkyGolaySmoothingEnabled(el.attribute("savitzkyGolaySmoothing") == "1"),
              morphologicalSmoothingEnabled(el.attribute("morphologicalSmoothing") == "1"),
              m_normalizeIllumination(el.attribute("normalizeIlluminationBW") == "1") {
    }

    QDomElement BlackWhiteOptions::toXml(QDomDocument& doc, QString const& name) const {
        QDomElement el(doc.createElement(name));
        el.setAttribute("thresholdAdj", m_thresholdAdjustment);
        el.setAttribute("savitzkyGolaySmoothing", savitzkyGolaySmoothingEnabled ? "1" : "0");
        el.setAttribute("morphologicalSmoothing", morphologicalSmoothingEnabled ? "1" : "0");
        el.setAttribute("normalizeIlluminationBW", m_normalizeIllumination ? "1" : "0");

        return el;
    }

    bool BlackWhiteOptions::operator==(BlackWhiteOptions const& other) const {
        return (m_thresholdAdjustment == other.m_thresholdAdjustment)
               && (savitzkyGolaySmoothingEnabled == other.savitzkyGolaySmoothingEnabled)
               && (morphologicalSmoothingEnabled == other.morphologicalSmoothingEnabled)
               && (m_normalizeIllumination == other.m_normalizeIllumination);
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
}  // namespace output