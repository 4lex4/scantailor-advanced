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

#include "DewarpingOptions.h"
#include <cassert>

namespace output {
DewarpingOptions::DewarpingOptions(DewarpingMode mode, bool needPostDeskew)
    : m_mode(mode), postDeskew(needPostDeskew) {}

DewarpingOptions::DewarpingOptions(const QDomElement& el)
    : m_mode(parseDewarpingMode(el.attribute("mode"))), postDeskew(el.attribute("postDeskew", "1") == "1") {}

QDomElement DewarpingOptions::toXml(QDomDocument& doc, const QString& name) const {
  QDomElement el(doc.createElement(name));
  el.setAttribute("mode", formatDewarpingMode(m_mode));
  el.setAttribute("postDeskew", postDeskew ? "1" : "0");

  return el;
}

bool DewarpingOptions::operator==(const DewarpingOptions& other) const {
  return (m_mode == other.m_mode) && (postDeskew == other.postDeskew);
}

bool DewarpingOptions::operator!=(const DewarpingOptions& other) const {
  return !(*this == other);
}

void DewarpingOptions::setDewarpingMode(DewarpingMode m_mode) {
  DewarpingOptions::m_mode = m_mode;
}

void DewarpingOptions::setPostDeskew(bool postDeskew) {
  DewarpingOptions::postDeskew = postDeskew;
}

bool DewarpingOptions::needPostDeskew() const {
  return postDeskew;
}

DewarpingMode DewarpingOptions::dewarpingMode() const {
  return m_mode;
}

DewarpingMode DewarpingOptions::parseDewarpingMode(const QString& str) {
  if (str == "auto") {
    return AUTO;
  } else if (str == "manual") {
    return MANUAL;
  } else if (str == "marginal") {
    return MARGINAL;
  } else {
    return OFF;
  }
}

QString DewarpingOptions::formatDewarpingMode(DewarpingMode mode) {
  switch (mode) {
    case OFF:
      return "off";
    case AUTO:
      return "auto";
    case MANUAL:
      return "manual";
    case MARGINAL:
      return "marginal";
  }

  assert(!"Unreachable");

  return QString();
}
}  // namespace output