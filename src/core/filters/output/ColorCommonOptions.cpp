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

#include "ColorCommonOptions.h"
#include <QDomDocument>

namespace output {
ColorCommonOptions::ColorCommonOptions()
    : m_fillOffcut(true), m_fillMargins(true), m_normalizeIllumination(false), m_fillingColor(FILL_BACKGROUND) {}

ColorCommonOptions::ColorCommonOptions(const QDomElement& el)
    : m_fillOffcut(el.attribute("fillOffcut") == "1"),
      m_fillMargins(el.attribute("fillMargins") == "1"),
      m_normalizeIllumination(el.attribute("normalizeIlluminationColor") == "1"),
      m_fillingColor(parseFillingColor(el.attribute("fillingColor"))),
      m_posterizationOptions(el.namedItem("posterization-options").toElement()) {}

QDomElement ColorCommonOptions::toXml(QDomDocument& doc, const QString& name) const {
  QDomElement el(doc.createElement(name));
  el.setAttribute("fillMargins", m_fillMargins ? "1" : "0");
  el.setAttribute("fillOffcut", m_fillOffcut ? "1" : "0");
  el.setAttribute("normalizeIlluminationColor", m_normalizeIllumination ? "1" : "0");
  el.setAttribute("fillingColor", formatFillingColor(m_fillingColor));
  el.appendChild(m_posterizationOptions.toXml(doc, "posterization-options"));

  return el;
}

bool ColorCommonOptions::operator==(const ColorCommonOptions& other) const {
  return (m_normalizeIllumination == other.m_normalizeIllumination) && (m_fillMargins == other.m_fillMargins)
         && (m_fillOffcut == other.m_fillOffcut) && (m_fillingColor == other.m_fillingColor)
         && (m_posterizationOptions == other.m_posterizationOptions);
}

bool ColorCommonOptions::operator!=(const ColorCommonOptions& other) const {
  return !(*this == other);
}

FillingColor ColorCommonOptions::getFillingColor() const {
  return m_fillingColor;
}

void ColorCommonOptions::setFillingColor(FillingColor fillingColor) {
  ColorCommonOptions::m_fillingColor = fillingColor;
}

FillingColor ColorCommonOptions::parseFillingColor(const QString& str) {
  if (str == "white") {
    return FILL_WHITE;
  } else {
    return FILL_BACKGROUND;
  }
}

QString ColorCommonOptions::formatFillingColor(FillingColor type) {
  QString str = "";
  switch (type) {
    case FILL_WHITE:
      str = "white";
      break;
    case FILL_BACKGROUND:
      str = "background";
      break;
  }

  return str;
}

void ColorCommonOptions::setFillMargins(bool val) {
  m_fillMargins = val;
}

bool ColorCommonOptions::fillMargins() const {
  return m_fillMargins;
}

bool ColorCommonOptions::normalizeIllumination() const {
  return m_normalizeIllumination;
}

void ColorCommonOptions::setNormalizeIllumination(bool val) {
  m_normalizeIllumination = val;
}

const ColorCommonOptions::PosterizationOptions& ColorCommonOptions::getPosterizationOptions() const {
  return m_posterizationOptions;
}

void ColorCommonOptions::setPosterizationOptions(const ColorCommonOptions::PosterizationOptions& posterizationOptions) {
  ColorCommonOptions::m_posterizationOptions = posterizationOptions;
}

bool ColorCommonOptions::fillOffcut() const {
  return m_fillOffcut;
}

void ColorCommonOptions::setFillOffcut(bool fillOffcut) {
  m_fillOffcut = fillOffcut;
}

/*=============================== ColorCommonOptions::PosterizationOptions ==================================*/

ColorCommonOptions::PosterizationOptions::PosterizationOptions()
    : m_isEnabled(false), m_level(4), m_isNormalizationEnabled(false), m_forceBlackAndWhite(true) {}

ColorCommonOptions::PosterizationOptions::PosterizationOptions(const QDomElement& el)
    : m_isEnabled(el.attribute("enabled") == "1"),
      m_level(el.attribute("level").toInt()),
      m_isNormalizationEnabled(el.attribute("normalizationEnabled") == "1"),
      m_forceBlackAndWhite(el.attribute("forceBlackAndWhite") == "1") {}

QDomElement ColorCommonOptions::PosterizationOptions::toXml(QDomDocument& doc, const QString& name) const {
  QDomElement el(doc.createElement(name));
  el.setAttribute("enabled", m_isEnabled ? "1" : "0");
  el.setAttribute("level", m_level);
  el.setAttribute("normalizationEnabled", m_isNormalizationEnabled ? "1" : "0");
  el.setAttribute("forceBlackAndWhite", m_forceBlackAndWhite ? "1" : "0");

  return el;
}

bool ColorCommonOptions::PosterizationOptions::operator==(const ColorCommonOptions::PosterizationOptions& other) const {
  return (m_isEnabled == other.m_isEnabled) && (m_level == other.m_level)
         && (m_isNormalizationEnabled == other.m_isNormalizationEnabled)
         && (m_forceBlackAndWhite == other.m_forceBlackAndWhite);
}

bool ColorCommonOptions::PosterizationOptions::operator!=(const ColorCommonOptions::PosterizationOptions& other) const {
  return !(*this == other);
}

bool ColorCommonOptions::PosterizationOptions::isEnabled() const {
  return m_isEnabled;
}

void ColorCommonOptions::PosterizationOptions::setEnabled(bool enabled) {
  PosterizationOptions::m_isEnabled = enabled;
}

int ColorCommonOptions::PosterizationOptions::getLevel() const {
  return m_level;
}

void ColorCommonOptions::PosterizationOptions::setLevel(int level) {
  PosterizationOptions::m_level = level;
}

bool ColorCommonOptions::PosterizationOptions::isNormalizationEnabled() const {
  return m_isNormalizationEnabled;
}

void ColorCommonOptions::PosterizationOptions::setNormalizationEnabled(bool normalizationEnabled) {
  PosterizationOptions::m_isNormalizationEnabled = normalizationEnabled;
}

bool ColorCommonOptions::PosterizationOptions::isForceBlackAndWhite() const {
  return m_forceBlackAndWhite;
}

void ColorCommonOptions::PosterizationOptions::setForceBlackAndWhite(bool forceBlackAndWhite) {
  PosterizationOptions::m_forceBlackAndWhite = forceBlackAndWhite;
}
}  // namespace output