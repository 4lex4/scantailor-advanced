// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "SplittingOptions.h"

namespace output {

SplittingOptions::SplittingOptions()
    : m_isSplitOutput(false), m_splittingMode(BLACK_AND_WHITE_FOREGROUND), m_isOriginalBackgroundEnabled(false) {}

SplittingOptions::SplittingOptions(const QDomElement& el)
    : m_isSplitOutput(el.attribute("splitOutput") == "1"),
      m_splittingMode(parseSplittingMode(el.attribute("splittingMode"))),
      m_isOriginalBackgroundEnabled(el.attribute("originalBackground") == "1") {}

QDomElement SplittingOptions::toXml(QDomDocument& doc, const QString& name) const {
  QDomElement el(doc.createElement(name));
  el.setAttribute("splitOutput", m_isSplitOutput ? "1" : "0");
  el.setAttribute("splittingMode", formatSplittingMode(m_splittingMode));
  el.setAttribute("originalBackground", m_isOriginalBackgroundEnabled ? "1" : "0");

  return el;
}

bool SplittingOptions::isSplitOutput() const {
  return m_isSplitOutput;
}

void SplittingOptions::setSplitOutput(bool splitOutput) {
  SplittingOptions::m_isSplitOutput = splitOutput;
}

SplittingMode SplittingOptions::getSplittingMode() const {
  return m_splittingMode;
}

void SplittingOptions::setSplittingMode(SplittingMode foregroundType) {
  SplittingOptions::m_splittingMode = foregroundType;
}

bool SplittingOptions::isOriginalBackgroundEnabled() const {
  return m_isOriginalBackgroundEnabled;
}

void SplittingOptions::setOriginalBackgroundEnabled(bool enable) {
  SplittingOptions::m_isOriginalBackgroundEnabled = enable;
}

SplittingMode SplittingOptions::parseSplittingMode(const QString& str) {
  if (str == "color") {
    return COLOR_FOREGROUND;
  } else {
    return BLACK_AND_WHITE_FOREGROUND;
  }
}

QString SplittingOptions::formatSplittingMode(const SplittingMode type) {
  QString str = "";
  switch (type) {
    case BLACK_AND_WHITE_FOREGROUND:
      str = "bw";
      break;
    case COLOR_FOREGROUND:
      str = "color";
      break;
  }

  return str;
}

bool SplittingOptions::operator==(const SplittingOptions& other) const {
  return (m_isSplitOutput == other.m_isSplitOutput) && (m_splittingMode == other.m_splittingMode)
         && (m_isOriginalBackgroundEnabled == other.m_isOriginalBackgroundEnabled);
}

bool SplittingOptions::operator!=(const SplittingOptions& other) const {
  return !(*this == other);
}
}  // namespace output