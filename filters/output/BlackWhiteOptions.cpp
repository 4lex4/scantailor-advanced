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
#include <cmath>
#include "../../Utils.h"

namespace output {
BlackWhiteOptions::BlackWhiteOptions()
    : m_thresholdAdjustment(0),
      m_savitzkyGolaySmoothingEnabled(true),
      m_morphologicalSmoothingEnabled(true),
      m_normalizeIllumination(true),
      m_windowSize(200),
      m_sauvolaCoef(0.34),
      m_wolfLowerBound(1),
      m_wolfUpperBound(254),
      m_wolfCoef(0.3),
      m_binarizationMethod(OTSU) {}

BlackWhiteOptions::BlackWhiteOptions(const QDomElement& el)
    : m_thresholdAdjustment(el.attribute("thresholdAdj").toInt()),
      m_savitzkyGolaySmoothingEnabled(el.attribute("savitzkyGolaySmoothing") == "1"),
      m_morphologicalSmoothingEnabled(el.attribute("morphologicalSmoothing") == "1"),
      m_normalizeIllumination(el.attribute("normalizeIlluminationBW") == "1"),
      m_windowSize(el.attribute("windowSize").toInt()),
      m_sauvolaCoef(el.attribute("sauvolaCoef").toDouble()),
      m_wolfLowerBound(el.attribute("wolfLowerBound").toInt()),
      m_wolfUpperBound(el.attribute("wolfUpperBound").toInt()),
      m_wolfCoef(el.attribute("wolfCoef").toDouble()),
      m_binarizationMethod(parseBinarizationMethod(el.attribute("binarizationMethod"))),
      m_colorSegmenterOptions(el.namedItem("color-segmenter-options").toElement()) {}

QDomElement BlackWhiteOptions::toXml(QDomDocument& doc, const QString& name) const {
  QDomElement el(doc.createElement(name));
  el.setAttribute("thresholdAdj", m_thresholdAdjustment);
  el.setAttribute("savitzkyGolaySmoothing", m_savitzkyGolaySmoothingEnabled ? "1" : "0");
  el.setAttribute("morphologicalSmoothing", m_morphologicalSmoothingEnabled ? "1" : "0");
  el.setAttribute("normalizeIlluminationBW", m_normalizeIllumination ? "1" : "0");
  el.setAttribute("windowSize", m_windowSize);
  el.setAttribute("sauvolaCoef", Utils::doubleToString(m_sauvolaCoef));
  el.setAttribute("wolfLowerBound", m_wolfLowerBound);
  el.setAttribute("wolfUpperBound", m_wolfUpperBound);
  el.setAttribute("wolfCoef", Utils::doubleToString(m_wolfCoef));
  el.setAttribute("binarizationMethod", formatBinarizationMethod(m_binarizationMethod));
  el.appendChild(m_colorSegmenterOptions.toXml(doc, "color-segmenter-options"));

  return el;
}

bool BlackWhiteOptions::operator==(const BlackWhiteOptions& other) const {
  return (m_thresholdAdjustment == other.m_thresholdAdjustment)
         && (m_savitzkyGolaySmoothingEnabled == other.m_savitzkyGolaySmoothingEnabled)
         && (m_morphologicalSmoothingEnabled == other.m_morphologicalSmoothingEnabled)
         && (m_normalizeIllumination == other.m_normalizeIllumination) && (m_windowSize == other.m_windowSize)
         && (m_sauvolaCoef == other.m_sauvolaCoef) && (m_wolfLowerBound == other.m_wolfLowerBound)
         && (m_wolfUpperBound == other.m_wolfUpperBound) && (m_wolfCoef == other.m_wolfCoef)
         && (m_binarizationMethod == other.m_binarizationMethod)
         && (m_colorSegmenterOptions == other.m_colorSegmenterOptions);
}

bool BlackWhiteOptions::operator!=(const BlackWhiteOptions& other) const {
  return !(*this == other);
}

bool BlackWhiteOptions::isSavitzkyGolaySmoothingEnabled() const {
  return m_savitzkyGolaySmoothingEnabled;
}

void BlackWhiteOptions::setSavitzkyGolaySmoothingEnabled(bool savitzkyGolaySmoothingEnabled) {
  BlackWhiteOptions::m_savitzkyGolaySmoothingEnabled = savitzkyGolaySmoothingEnabled;
}

bool BlackWhiteOptions::isMorphologicalSmoothingEnabled() const {
  return m_morphologicalSmoothingEnabled;
}

void BlackWhiteOptions::setMorphologicalSmoothingEnabled(bool morphologicalSmoothingEnabled) {
  BlackWhiteOptions::m_morphologicalSmoothingEnabled = morphologicalSmoothingEnabled;
}

int BlackWhiteOptions::getWindowSize() const {
  return m_windowSize;
}

void BlackWhiteOptions::setWindowSize(int windowSize) {
  BlackWhiteOptions::m_windowSize = windowSize;
}

double BlackWhiteOptions::getSauvolaCoef() const {
  return m_sauvolaCoef;
}

void BlackWhiteOptions::setSauvolaCoef(double sauvolaCoef) {
  BlackWhiteOptions::m_sauvolaCoef = sauvolaCoef;
}

int BlackWhiteOptions::getWolfLowerBound() const {
  return m_wolfLowerBound;
}

void BlackWhiteOptions::setWolfLowerBound(int wolfLowerBound) {
  BlackWhiteOptions::m_wolfLowerBound = wolfLowerBound;
}

int BlackWhiteOptions::getWolfUpperBound() const {
  return m_wolfUpperBound;
}

void BlackWhiteOptions::setWolfUpperBound(int wolfUpperBound) {
  BlackWhiteOptions::m_wolfUpperBound = wolfUpperBound;
}

double BlackWhiteOptions::getWolfCoef() const {
  return m_wolfCoef;
}

void BlackWhiteOptions::setWolfCoef(double wolfCoef) {
  BlackWhiteOptions::m_wolfCoef = wolfCoef;
}

BinarizationMethod BlackWhiteOptions::getBinarizationMethod() const {
  return m_binarizationMethod;
}

void BlackWhiteOptions::setBinarizationMethod(BinarizationMethod binarizationMethod) {
  BlackWhiteOptions::m_binarizationMethod = binarizationMethod;
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

const BlackWhiteOptions::ColorSegmenterOptions& BlackWhiteOptions::getColorSegmenterOptions() const {
  return m_colorSegmenterOptions;
}

void BlackWhiteOptions::setColorSegmenterOptions(
    const BlackWhiteOptions::ColorSegmenterOptions& colorSegmenterOptions) {
  BlackWhiteOptions::m_colorSegmenterOptions = colorSegmenterOptions;
}

/*=============================== BlackWhiteOptions::ColorSegmenterOptions ==================================*/

BlackWhiteOptions::ColorSegmenterOptions::ColorSegmenterOptions()
    : m_isEnabled(false),
      m_noiseReduction(7),
      m_redThresholdAdjustment(0),
      m_greenThresholdAdjustment(0),
      m_blueThresholdAdjustment(0) {}

BlackWhiteOptions::ColorSegmenterOptions::ColorSegmenterOptions(const QDomElement& el)
    : m_isEnabled(el.attribute("enabled") == "1"),
      m_noiseReduction(el.attribute("noiseReduction").toInt()),
      m_redThresholdAdjustment(el.attribute("redThresholdAdjustment").toInt()),
      m_greenThresholdAdjustment(el.attribute("greenThresholdAdjustment").toInt()),
      m_blueThresholdAdjustment(el.attribute("blueThresholdAdjustment").toInt()) {}

QDomElement BlackWhiteOptions::ColorSegmenterOptions::toXml(QDomDocument& doc, const QString& name) const {
  QDomElement el(doc.createElement(name));
  el.setAttribute("enabled", m_isEnabled ? "1" : "0");
  el.setAttribute("noiseReduction", m_noiseReduction);
  el.setAttribute("redThresholdAdjustment", m_redThresholdAdjustment);
  el.setAttribute("greenThresholdAdjustment", m_greenThresholdAdjustment);
  el.setAttribute("blueThresholdAdjustment", m_blueThresholdAdjustment);

  return el;
}

bool BlackWhiteOptions::ColorSegmenterOptions::operator==(const BlackWhiteOptions::ColorSegmenterOptions& other) const {
  return (m_isEnabled == other.m_isEnabled) && (m_noiseReduction == other.m_noiseReduction)
         && (m_redThresholdAdjustment == other.m_redThresholdAdjustment)
         && (m_greenThresholdAdjustment == other.m_greenThresholdAdjustment)
         && (m_blueThresholdAdjustment == other.m_blueThresholdAdjustment);
}

bool BlackWhiteOptions::ColorSegmenterOptions::operator!=(const BlackWhiteOptions::ColorSegmenterOptions& other) const {
  return !(*this == other);
}

bool BlackWhiteOptions::ColorSegmenterOptions::isEnabled() const {
  return m_isEnabled;
}

void BlackWhiteOptions::ColorSegmenterOptions::setEnabled(bool enabled) {
  ColorSegmenterOptions::m_isEnabled = enabled;
}

int BlackWhiteOptions::ColorSegmenterOptions::getNoiseReduction() const {
  return m_noiseReduction;
}

void BlackWhiteOptions::ColorSegmenterOptions::setNoiseReduction(int noiseReduction) {
  ColorSegmenterOptions::m_noiseReduction = noiseReduction;
}

int BlackWhiteOptions::ColorSegmenterOptions::getRedThresholdAdjustment() const {
  return m_redThresholdAdjustment;
}

void BlackWhiteOptions::ColorSegmenterOptions::setRedThresholdAdjustment(int redThresholdAdjustment) {
  ColorSegmenterOptions::m_redThresholdAdjustment = redThresholdAdjustment;
}

int BlackWhiteOptions::ColorSegmenterOptions::getGreenThresholdAdjustment() const {
  return m_greenThresholdAdjustment;
}

void BlackWhiteOptions::ColorSegmenterOptions::setGreenThresholdAdjustment(int greenThresholdAdjustment) {
  ColorSegmenterOptions::m_greenThresholdAdjustment = greenThresholdAdjustment;
}

int BlackWhiteOptions::ColorSegmenterOptions::getBlueThresholdAdjustment() const {
  return m_blueThresholdAdjustment;
}

void BlackWhiteOptions::ColorSegmenterOptions::setBlueThresholdAdjustment(int blueThresholdAdjustment) {
  ColorSegmenterOptions::m_blueThresholdAdjustment = blueThresholdAdjustment;
}

}  // namespace output