// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "OutputProcessingParams.h"
#include <QDomDocument>
#include "BlackWhiteOptions.h"

namespace output {

OutputProcessingParams::OutputProcessingParams() : m_autoZonesFound(false), m_blackOnWhiteSetManually(false) {}

OutputProcessingParams::OutputProcessingParams(const QDomElement& el)
    : m_autoZonesFound(el.attribute("autoZonesFound") == "1"),
      m_blackOnWhiteSetManually(el.attribute("blackOnWhiteSetManually") == "1") {}

QDomElement OutputProcessingParams::toXml(QDomDocument& doc, const QString& name) const {
  QDomElement el(doc.createElement(name));
  el.setAttribute("autoZonesFound", m_autoZonesFound ? "1" : "0");
  el.setAttribute("blackOnWhiteSetManually", m_blackOnWhiteSetManually ? "1" : "0");

  return el;
}

bool OutputProcessingParams::operator==(const OutputProcessingParams& other) const {
  return (m_autoZonesFound == other.m_autoZonesFound) && (m_blackOnWhiteSetManually == other.m_blackOnWhiteSetManually);
}

bool OutputProcessingParams::operator!=(const OutputProcessingParams& other) const {
  return !(*this == other);
}

bool output::OutputProcessingParams::isAutoZonesFound() const {
  return m_autoZonesFound;
}

void output::OutputProcessingParams::setAutoZonesFound(bool autoZonesFound) {
  OutputProcessingParams::m_autoZonesFound = autoZonesFound;
}

bool OutputProcessingParams::isBlackOnWhiteSetManually() const {
  return m_blackOnWhiteSetManually;
}

void OutputProcessingParams::setBlackOnWhiteSetManually(bool blackOnWhiteSetManually) {
  OutputProcessingParams::m_blackOnWhiteSetManually = blackOnWhiteSetManually;
}
}  // namespace output