// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "ColorParams.h"

namespace output {
ColorParams::ColorParams() : m_colorMode(BLACK_AND_WHITE) {}

ColorParams::ColorParams(const QDomElement& el)
    : m_colorMode(parseColorMode(el.attribute("colorMode"))),
      m_colorCommonOptions(el.namedItem("color-or-grayscale").toElement()),
      m_bwOptions(el.namedItem("bw").toElement()) {}

QDomElement ColorParams::toXml(QDomDocument& doc, const QString& name) const {
  QDomElement el(doc.createElement(name));
  el.setAttribute("colorMode", formatColorMode(m_colorMode));
  el.appendChild(m_colorCommonOptions.toXml(doc, "color-or-grayscale"));
  el.appendChild(m_bwOptions.toXml(doc, "bw"));
  return el;
}

ColorMode ColorParams::parseColorMode(const QString& str) {
  if (str == "bw") {
    return BLACK_AND_WHITE;
  } else if (str == "colorOrGray") {
    return COLOR_GRAYSCALE;
  } else if (str == "mixed") {
    return MIXED;
  } else {
    return BLACK_AND_WHITE;
  }
}

QString ColorParams::formatColorMode(const ColorMode mode) {
  const char* str = "";
  switch (mode) {
    case BLACK_AND_WHITE:
      str = "bw";
      break;
    case COLOR_GRAYSCALE:
      str = "colorOrGray";
      break;
    case MIXED:
      str = "mixed";
      break;
  }
  return QString::fromLatin1(str);
}

ColorMode ColorParams::colorMode() const {
  return m_colorMode;
}

void ColorParams::setColorMode(ColorMode mode) {
  m_colorMode = mode;
}

const ColorCommonOptions& ColorParams::colorCommonOptions() const {
  return m_colorCommonOptions;
}

void ColorParams::setColorCommonOptions(const ColorCommonOptions& opt) {
  m_colorCommonOptions = opt;
}

const BlackWhiteOptions& ColorParams::blackWhiteOptions() const {
  return m_bwOptions;
}

void ColorParams::setBlackWhiteOptions(const BlackWhiteOptions& opt) {
  m_bwOptions = opt;
}
}  // namespace output